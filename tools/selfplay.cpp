// In-process A/B match between two Regicide configurations.
//
//   selfplay --a "lmr_mult=50" --b "" --nodes 20000 --threads 12 --pairs 300
//
// Every distinct position after 3 plies from the start is an opening; each is played twice with colours
// reversed. Reports W/D/L for A, and Elo with a 95% interval from the pentanomial distribution of game pairs.
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

struct Config {
    std::string a, b;
    Limits limits;
    int threads = 8;
    int pairs = 0;  // 0 = every opening once
    std::size_t hash_mb = 16;
    int adjudicate_score = 600;
    int adjudicate_plies = 8;
    int draw_reversible = 80;
    int max_plies = 400;
    bool verbose = false;
};

std::vector<Position> three_ply_openings() {
    std::vector<Position> out;
    std::set<Key> seen;
    auto rec = [&](auto& self, const Position& p, int depth) -> void {
        if (depth == 0) {
            if (seen.insert(p.key()).second) out.push_back(p);
            return;
        }
        MoveList list;
        generate_moves(p, list);
        for (const Move& m : list) {
            Position c = p;
            c.make_move(m);
            self(self, c, depth - 1);
        }
    };
    rec(rec, Position(), 3);
    return out;
}

// Plays one game; returns black's score (1, 0.5, 0).
double play_game(const Position& start, Engine& black, Engine& white, const Config& cfg) {
    Position pos = start;
    std::vector<Position> history;
    black.new_game();
    white.new_game();
    int decisive_run = 0, last_sign = 0;

    for (int ply = 0; ply < cfg.max_plies; ++ply) {
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) return pos.side_to_move() == BLACK ? 0.0 : 1.0;
        if (pos.reversible_plies() >= cfg.draw_reversible) return 0.5;
        int reps = 0;
        for (const Position& h : history) reps += h == pos;
        if (reps >= 2) return 0.5;

        Engine& e = pos.side_to_move() == BLACK ? black : white;
        e.set_position(pos, history);
        SearchResult r = e.go(cfg.limits);

        // Adjudicate when both sides agree the game is decided for several plies.
        const int black_view = pos.side_to_move() == BLACK ? r.score : -r.score;
        const int sign = black_view >= cfg.adjudicate_score ? 1 : black_view <= -cfg.adjudicate_score ? -1 : 0;
        decisive_run = sign != 0 && sign == last_sign ? decisive_run + 1 : sign != 0;
        last_sign = sign;
        if (cfg.adjudicate_score > 0 && decisive_run >= cfg.adjudicate_plies) return sign > 0 ? 1.0 : 0.0;

        history.push_back(pos);
        pos.make_move(r.best);
    }
    return 0.5;
}

void report(const char* tag, const std::vector<int>& penta, int w, int d, int l) {
    const int n = w + d + l, pairs = penta[0] + penta[1] + penta[2] + penta[3] + penta[4];
    if (!n || !pairs) return;
    const double score = (w + 0.5 * d) / n;
    double mean = 0, var = 0;
    for (int i = 0; i < 5; ++i) mean += penta[i] * (i / 4.0);
    mean /= pairs;
    for (int i = 0; i < 5; ++i) var += penta[i] * std::pow(i / 4.0 - mean, 2);
    var /= pairs;
    auto elo = [](double s) {
        s = std::clamp(s, 1e-6, 1 - 1e-6);
        return -400.0 * std::log10(1.0 / s - 1.0);
    };
    const double se = std::sqrt(var / pairs);
    const double lo = elo(mean - 1.96 * se), hi = elo(mean + 1.96 * se);
    std::printf("%s games %d  A: +%d =%d -%d  score %.4f  draws %.1f%%  elo %+.1f [%+.1f, %+.1f]  penta [%d %d %d %d %d]\n",
                tag, n, w, d, l, score, 100.0 * d / n, elo(score), lo, hi, penta[0], penta[1], penta[2], penta[3],
                penta[4]);
    std::fflush(stdout);
}

}  // namespace

int main(int argc, char** argv) {
    Config cfg;
    cfg.limits.nodes = 20000;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--a") cfg.a = next();
        else if (k == "--b") cfg.b = next();
        else if (k == "--nodes") cfg.limits.nodes = std::stoull(next());
        else if (k == "--depth") cfg.limits.depth = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--movetime") cfg.limits.movetime_ms = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--target") cfg.limits.target_ms = std::stoi(next()), cfg.limits.nodes = 0;
        else if (k == "--threads") cfg.threads = std::stoi(next());
        else if (k == "--pairs") cfg.pairs = std::stoi(next());
        else if (k == "--hash") cfg.hash_mb = std::stoull(next());
        else if (k == "--adjudicate") cfg.adjudicate_score = std::stoi(next());
        else if (k == "-v") cfg.verbose = true;
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }

    const std::vector<Position> openings = three_ply_openings();
    const int pairs = cfg.pairs ? cfg.pairs : int(openings.size());
    std::printf("openings %zu, pairs %d, A=\"%s\" B=\"%s\"\n", openings.size(), pairs, cfg.a.c_str(),
                cfg.b.c_str());

    std::atomic<int> next_pair{0};
    std::mutex mu;
    std::vector<int> penta(5, 0);
    int w = 0, d = 0, l = 0, done = 0;

    auto worker = [&] {
        Engine ea, eb;
        ea.set_hash_mb(cfg.hash_mb);
        eb.set_hash_mb(cfg.hash_mb);
        if (!ea.params.set_all(cfg.a) || !eb.params.set_all(cfg.b)) {
            std::cerr << "bad parameter spec\n";
            std::exit(2);
        }
        for (int i; (i = next_pair++) < pairs;) {
            const Position& open = openings[i % openings.size()];
            const double g1 = play_game(open, ea, eb, cfg);        // A is black
            const double g2 = 1.0 - play_game(open, eb, ea, cfg);  // A is white
            std::lock_guard lock(mu);
            for (double g : {g1, g2}) (g == 1.0 ? w : g == 0.0 ? l : d)++;
            penta[int(std::lround((g1 + g2) * 2))]++;
            if (++done % 20 == 0 || done == pairs) report(done == pairs ? "final" : "     ", penta, w, d, l);
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < cfg.threads; ++t) pool.emplace_back(worker);
    for (auto& t : pool) t.join();
    return 0;
}
