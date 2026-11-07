#pragma once

// Match statistics. Openings are played as pairs with colours reversed, so the statistical unit
// is the game pair (pentanomial model): its score for engine A is 0, 0.5, 1, 1.5 or 2.
// Pairing removes most of the opening bias, which matters in checkers where many ballots are
// lopsided and most games are drawn.

#include <array>
#include <optional>
#include <string>

namespace match {

struct Estimate {
    double score = 0.5;   // mean score per game for A
    double se = 0;        // standard error of `score`
    double elo = 0, elo_lo = 0, elo_hi = 0;  // logistic Elo and 95% interval
    double los = 0.5;     // likelihood of superiority, P(true score > 0.5)
    long n = 0;           // samples (pairs or games)
};

struct SprtConfig {
    double elo0 = 0, elo1 = 5, alpha = 0.05, beta = 0.05;
    // The normal approximation is meaningless for a handful of pairs (with identical pair
    // results the variance is 0 and the LLR explodes), so never stop before this many pairs.
    long min_pairs = 20;
    double lower() const;  // log(beta / (1 - alpha))
    double upper() const;  // log((1 - beta) / alpha)
};

struct Stats {
    long wins = 0, draws = 0, losses = 0;     // games, from A's point of view
    std::array<long, 5> penta{};              // complete pairs by A's pair score x2: LL, LD, DD|WL, WD, WW

    long games() const { return wins + draws + losses; }
    double draw_rate() const { return games() ? double(draws) / games() : 0; }
    long pairs() const { return penta[0] + penta[1] + penta[2] + penta[3] + penta[4]; }

    Estimate pentanomial() const;
    Estimate trinomial() const;
    // Generalized SPRT (normal approximation of the pentanomial pair scores, with the
    // variance estimated from the data): log-likelihood ratio of elo1 vs elo0.
    double llr(const SprtConfig& c) const;
};

double elo_from_score(double s);
double score_from_elo(double elo);
std::string format_elo(const Estimate& e);

}  // namespace match
