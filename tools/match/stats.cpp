#include "stats.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace match {

namespace {
constexpr double kZ95 = 1.959963984540054;

Estimate estimate(const double* values, const long* counts, int k) {
    Estimate e;
    long n = 0;
    double sum = 0;
    for (int i = 0; i < k; ++i) {
        n += counts[i];
        sum += counts[i] * values[i];
    }
    e.n = n;
    if (n == 0) return e;
    double mean = sum / n, var = 0;
    for (int i = 0; i < k; ++i) var += counts[i] * (values[i] - mean) * (values[i] - mean);
    var /= n;
    e.score = mean;
    e.se = std::sqrt(var / n);
    e.elo = elo_from_score(mean);
    e.elo_lo = elo_from_score(mean - kZ95 * e.se);
    e.elo_hi = elo_from_score(mean + kZ95 * e.se);
    e.los = e.se > 0 ? 0.5 * (1 + std::erf((mean - 0.5) / (e.se * std::sqrt(2.0)))) : (mean > 0.5 ? 1 : mean < 0.5 ? 0 : 0.5);
    return e;
}
}  // namespace

double elo_from_score(double s) {
    s = std::clamp(s, 1e-6, 1 - 1e-6);
    return -400 * std::log10(1 / s - 1);
}

double score_from_elo(double elo) { return 1 / (1 + std::pow(10.0, -elo / 400)); }

double SprtConfig::lower() const { return std::log(beta / (1 - alpha)); }
double SprtConfig::upper() const { return std::log((1 - beta) / alpha); }

Estimate Stats::pentanomial() const {
    const double v[5] = {0, 0.25, 0.5, 0.75, 1};
    return estimate(v, penta.data(), 5);
}

Estimate Stats::trinomial() const {
    const double v[3] = {0, 0.5, 1};
    const long c[3] = {losses, draws, wins};
    return estimate(v, c, 3);
}

double Stats::llr(const SprtConfig& c) const {
    const long n = pairs();
    if (n < 2) return 0;
    Estimate e = pentanomial();
    // Per-pair variance, estimated with two extra pairs a quarter point above and below the mean. Without them a
    // run of identical pairs (20 drawn pairs are common here) has variance 0, and any floor far below the real
    // variance turns that run into strong evidence: the old floor of 1e-4 accepted H0 = 0 against H1 = +5 Elo
    // after 20 drawn pairs.
    const double var = (e.se * e.se * n * n + 2 * 0.25 * 0.25) / (n + 2);
    const double s0 = score_from_elo(c.elo0), s1 = score_from_elo(c.elo1);
    return n * (s1 - s0) * (2 * e.score - s0 - s1) / (2 * var);
}

std::string format_elo(const Estimate& e) {
    char buf[128];
    std::snprintf(buf, sizeof buf, "%+.1f [%+.1f, %+.1f]", e.elo, e.elo_lo, e.elo_hi);
    return buf;
}

}  // namespace match
