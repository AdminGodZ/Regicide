// Trains the pattern evaluation (see features:: in eval.hpp) by logistic regression on self-play data.
//
//   train --data a.bin [--data b.bin ...] --out src/weights/eval.bin [--epochs 12] [--lambda 0.7]
//
// Target: lambda * game result + (1 - lambda) * sigmoid(search score / K). K is fitted to the data first so that
// the learned evaluation keeps the search's centi-man scale.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <numeric>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "datafile.hpp"
#include "eval.hpp"

using namespace regicide;

namespace {

struct Config {
    std::vector<std::string> data;
    std::string out = "eval.bin";
    std::string init;  // optional starting weights (RGEV file)
    int epochs = 12;
    int batch = 16384;
    int threads = 12;
    double lambda = 0.7;
    double lr = 2.0;
    double l2 = 1e-6;        // L2 added to the gradient (coupled; Adam rescales it, see the update below)
    double l2_table = 1e-4;
    double wd = 0;           // decoupled weight decay (AdamW): w -= lr * wd * w each step
    double wd_table = 0;
    double valid_frac = 0.02;
    std::uint64_t seed = 7;
    double k = 0;  // 0 = fit
};

double sigmoid(double x) { return 1.0 / (1.0 + std::exp(-x)); }

std::vector<DataRecord> load(const std::vector<std::string>& files) {
    std::vector<DataRecord> all;
    for (const std::string& f : files) {
        std::FILE* in = std::fopen(f.c_str(), "rb");
        if (!in) {
            std::cerr << "cannot open " << f << '\n';
            std::exit(1);
        }
        std::fseek(in, 0, SEEK_END);
        const long bytes = std::ftell(in);
        std::fseek(in, 0, SEEK_SET);
        const std::size_t n = std::size_t(bytes) / sizeof(DataRecord), old = all.size();
        all.resize(old + n);
        if (std::fread(all.data() + old, sizeof(DataRecord), n, in) != n) std::exit(1);
        std::fclose(in);
        std::printf("loaded %zu positions from %s\n", n, f.c_str());
    }
    return all;
}

// Fits K so that sigmoid(score / K) best predicts the game results (cross-entropy).
double fit_k(const std::vector<DataRecord>& d) {
    auto loss = [&](double k) {
        double l = 0;
        const std::size_t step = std::max<std::size_t>(1, d.size() / 2000000);
        std::size_t n = 0;
        for (std::size_t i = 0; i < d.size(); i += step, ++n) {
            const double p = std::clamp(sigmoid(d[i].score / k), 1e-9, 1 - 1e-9), t = d[i].stm_result();
            l -= t * std::log(p) + (1 - t) * std::log(1 - p);
        }
        return l / n;
    };
    double lo = 20, hi = 2000;
    for (int it = 0; it < 60; ++it) {
        const double m1 = lo + (hi - lo) / 3, m2 = hi - (hi - lo) / 3;
        if (loss(m1) < loss(m2))
            hi = m2;
        else
            lo = m1;
    }
    return (lo + hi) / 2;
}

struct Model {
    std::vector<float> w;  // [feature][phase]

    double predict(const features::List& f) const {
        double mg = 0, eg = 0;
        for (int i = 0; i < f.n; ++i) {
            mg += w[2 * f.f[i].index] * f.f[i].value;
            eg += w[2 * f.f[i].index + 1] * f.f[i].value;
        }
        const double ph = std::min(f.pieces, 24) / 24.0;
        return mg * ph + eg * (1 - ph);
    }
};

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : "0"; };
        if (k == "--data") cfg.data.push_back(next());
        else if (k == "--out") cfg.out = next();
        else if (k == "--init") cfg.init = next();
        else if (k == "--epochs") cfg.epochs = std::stoi(next());
        else if (k == "--batch") cfg.batch = std::stoi(next());
        else if (k == "--threads") cfg.threads = std::stoi(next());
        else if (k == "--lambda") cfg.lambda = std::stod(next());
        else if (k == "--lr") cfg.lr = std::stod(next());
        else if (k == "--l2") cfg.l2 = std::stod(next());
        else if (k == "--l2-table") cfg.l2_table = std::stod(next());
        else if (k == "--wd") cfg.wd = std::stod(next());
        else if (k == "--wd-table") cfg.wd_table = std::stod(next());
        else if (k == "--k") cfg.k = std::stod(next());
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }

    std::vector<DataRecord> data = load(cfg.data);
    std::mt19937_64 rng(cfg.seed);
    std::shuffle(data.begin(), data.end(), rng);
    const std::size_t nvalid = std::size_t(data.size() * cfg.valid_frac), ntrain = data.size() - nvalid;
    const double K = cfg.k > 0 ? cfg.k : fit_k(data);
    std::printf("train %zu  valid %zu  K %.1f  lambda %.2f\n", ntrain, nvalid, K, cfg.lambda);

    Model model;
    model.w.assign(std::size_t(features::COUNT) * 2, 0.0f);
    if (!cfg.init.empty()) {
        auto ev = Evaluator::from_file(cfg.init);
        if (!ev) {
            std::cerr << "cannot load " << cfg.init << '\n';
            return 1;
        }
        // Re-read the raw weights through a fresh load of the file.
        std::FILE* in = std::fopen(cfg.init.c_str(), "rb");
        std::fseek(in, 12, SEEK_SET);
        std::vector<std::int16_t> q(model.w.size());
        if (std::fread(q.data(), 2, q.size(), in) != q.size()) return 1;
        std::fclose(in);
        for (std::size_t i = 0; i < q.size(); ++i) model.w[i] = float(q[i]) / Evaluator::SCALE;
    } else {
        for (int ph = 0; ph < 2; ++ph) {
            model.w[2 * (features::MATERIAL + 0) + ph] = 100;
            model.w[2 * (features::MATERIAL + 1) + ph] = 130;
            model.w[2 * (features::MATERIAL + 2) + ph] = -100;
            model.w[2 * (features::MATERIAL + 3) + ph] = -130;
        }
    }

    auto target = [&](const DataRecord& r) {
        return cfg.lambda * r.stm_result() + (1 - cfg.lambda) * sigmoid(r.score / K);
    };
    auto is_table = [&](int feature) { return feature >= features::MATERIAL_TABLE && feature < features::KING_SQ; };

    auto validate = [&] {
        std::vector<double> part(cfg.threads, 0.0);
        std::vector<std::thread> pool;
        for (int t = 0; t < cfg.threads; ++t)
            pool.emplace_back([&, t] {
                features::List f;
                for (std::size_t i = ntrain + t; i < data.size(); i += cfg.threads) {
                    features::extract(data[i].position(), f);
                    const double p = std::clamp(sigmoid(model.predict(f) / K), 1e-9, 1 - 1e-9), y = target(data[i]);
                    part[t] -= y * std::log(p) + (1 - y) * std::log(1 - p);
                }
            });
        for (auto& th : pool) th.join();
        return std::accumulate(part.begin(), part.end(), 0.0) / std::max<std::size_t>(1, nvalid);
    };

    const std::size_t nw = model.w.size();
    std::vector<float> m1(nw, 0), m2(nw, 0), grad(nw, 0);
    std::vector<std::vector<float>> tgrad(cfg.threads, std::vector<float>(nw, 0));
    std::vector<std::vector<int>> touched(cfg.threads);
    std::vector<float> l2_of(nw), wd_of(nw);
    for (std::size_t k = 0; k < nw; ++k) {
        l2_of[k] = float(is_table(int(k / 2)) ? cfg.l2_table : cfg.l2);
        wd_of[k] = float(is_table(int(k / 2)) ? cfg.wd_table : cfg.wd);
    }
    std::vector<std::size_t> order(ntrain);
    std::iota(order.begin(), order.end(), 0);
    const double b1 = 0.9, b2 = 0.999, eps = 1e-8;
    long step = 0;

    std::printf("epoch 0  valid loss %.6f\n", validate());
    for (int epoch = 1; epoch <= cfg.epochs; ++epoch) {
        std::shuffle(order.begin(), order.end(), rng);
        const double lr = cfg.lr * (epoch > cfg.epochs * 2 / 3 ? 0.1 : epoch > cfg.epochs / 3 ? 0.3 : 1.0);
        double train_loss = 0;
        for (std::size_t start = 0; start < ntrain; start += cfg.batch) {
            const std::size_t end = std::min(ntrain, start + cfg.batch);
            std::vector<double> part(cfg.threads, 0.0);
            std::vector<std::thread> pool;
            for (int t = 0; t < cfg.threads; ++t)
                pool.emplace_back([&, t] {
                    features::List f;
                    std::vector<float>& g = tgrad[t];
                    for (std::size_t j = start + t; j < end; j += cfg.threads) {
                        const DataRecord& r = data[order[j]];
                        features::extract(r.position(), f);
                        const double p = sigmoid(model.predict(f) / K), y = target(r);
                        part[t] -= y * std::log(std::max(p, 1e-9)) + (1 - y) * std::log(std::max(1 - p, 1e-9));
                        const double d = (p - y) / K;
                        const double ph = std::min(f.pieces, 24) / 24.0;
                        for (int i = 0; i < f.n; ++i) {
                            const int idx = f.f[i].index;
                            g[2 * idx] += float(d * f.f[i].value * ph);
                            g[2 * idx + 1] += float(d * f.f[i].value * (1 - ph));
                            touched[t].push_back(idx);
                        }
                    }
                });
            for (auto& th : pool) th.join();
            for (double x : part) train_loss += x;

            // Reduce the per-thread gradients (only touched entries are non-zero).
            for (int t = 0; t < cfg.threads; ++t) {
                for (int idx : touched[t])
                    for (int ph = 0; ph < 2; ++ph) {
                        grad[2 * idx + ph] += tgrad[t][2 * idx + ph];
                        tgrad[t][2 * idx + ph] = 0;
                    }
                touched[t].clear();
            }
            // Dense Adam step: every weight's moments decay every step, as in standard Adam. Coupled L2 on a weight
            // whose data gradient is rare gets normalised to a full-size step towards zero (about lr per batch), so
            // prefer --l2 0 with decoupled --wd, which shrinks in proportion to the weight instead.
            ++step;
            const double bs = double(end - start);
            const double c1 = 1 - std::pow(b1, step), c2 = 1 - std::pow(b2, step);
            for (std::size_t k = 0; k < nw; ++k) {
                const double g = grad[k] / bs + l2_of[k] * model.w[k];
                grad[k] = 0;
                m1[k] = float(b1 * m1[k] + (1 - b1) * g);
                m2[k] = float(b2 * m2[k] + (1 - b2) * g * g);
                model.w[k] -= float(lr * ((m1[k] / c1) / (std::sqrt(m2[k] / c2) + eps) + wd_of[k] * model.w[k]));
            }
        }
        std::printf("epoch %d  lr %.3f  train loss %.6f  valid loss %.6f\n", epoch, lr, train_loss / ntrain,
                    validate());
        std::fflush(stdout);
    }

    // Export: int16 in 1/SCALE centi-men.
    std::FILE* out = std::fopen(cfg.out.c_str(), "wb");
    if (!out) return 1;
    const std::uint32_t version = 1, count = features::COUNT;
    std::fwrite("RGEV", 1, 4, out);
    std::fwrite(&version, 4, 1, out);
    std::fwrite(&count, 4, 1, out);
    long clipped = 0;
    for (float x : model.w) {
        const long v = std::lround(x * Evaluator::SCALE);
        clipped += v > 32767 || v < -32767;
        const std::int16_t q = std::int16_t(std::clamp(v, -32767L, 32767L));
        std::fwrite(&q, 2, 1, out);
    }
    std::fclose(out);
    std::printf("wrote %s (%ld weights clipped)\n", cfg.out.c_str(), clipped);
    std::printf("material mg: man %.1f king %.1f | eg: man %.1f king %.1f\n", model.w[2 * features::MATERIAL],
                model.w[2 * (features::MATERIAL + 1)], model.w[2 * features::MATERIAL + 1],
                model.w[2 * (features::MATERIAL + 1) + 1]);
    return 0;
}
