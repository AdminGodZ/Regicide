#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "position.hpp"

namespace regicide {

namespace nnue {
class Network;
}

using Score = int;

constexpr int MAX_PLY = 128;

constexpr Score SCORE_INF = 32000;
constexpr Score SCORE_WIN = 30000;                    // minus distance to the win, in plies
constexpr Score SCORE_MATE_BOUND = SCORE_WIN - 1000;  // above: a forced win by running out of moves
constexpr Score SCORE_DB_WIN = 20000;                 // endgame-database win, minus distance in plies
constexpr Score SCORE_KNOWN_WIN = 15000;              // above: proven win (mate or database)
constexpr Score SCORE_DRAW = 0;

constexpr bool is_decisive(Score s) { return s >= SCORE_KNOWN_WIN || s <= -SCORE_KNOWN_WIN; }

// Hand-crafted evaluation, used when no trained weights are available.
Score evaluate_hce(const Position& pos);

// Learned evaluation: a linear model over sparse features of the position seen from the side to move, which
// is always oriented to move "up" the board (black positions are rotated 180 degrees with colours swapped).
// Each feature has a middlegame and an endgame weight, blended by the number of pieces on the board.
namespace features {

// Optional feature sets, selected at build time (the weights file records the feature count, so weights trained
// for one set are rejected by another):
//   REGICIDE_EVAL_BANDS: full-width two-row bands (rows r, r+1) as extra patterns (default; +11 Elo, see
//   experiments/eval-gen1). A king-split window set was also tried and lost ~7 Elo.
#ifndef REGICIDE_EVAL_BANDS
#define REGICIDE_EVAL_BANDS 1
#endif

constexpr int WINDOWS = 25;       // 4x4 board windows at row offsets 0..4 and column offsets 0..4
constexpr int WINDOW_STATES = 6561;  // 3^8: each of the 8 dark squares is empty, ours or theirs
constexpr int BANDS = REGICIDE_EVAL_BANDS ? 7 : 0;  // two-row bands starting at rows 0..6 (8 squares each)

constexpr int PATTERN = 0;
constexpr int BAND = PATTERN + WINDOWS * WINDOW_STATES;
constexpr int MATERIAL = BAND + BANDS * WINDOW_STATES;  // our men, our kings, their men, their kings (counts)
constexpr int MATERIAL_TABLE = MATERIAL + 4;                 // [our men][our kings][their men][their kings]
constexpr int KING_SQ = MATERIAL_TABLE + 13 * 13 * 13 * 13;  // our kings by square, then theirs
constexpr int MOBILITY = KING_SQ + 64;                       // our quiet-move count (0..23), then theirs
constexpr int RUNAWAY = MOBILITY + 48;                       // unopposed men by rows left to crown, ours then theirs
constexpr int TRAPPED_KINGS = RUNAWAY + 14;                  // kings without a quiet move, ours then theirs
constexpr int THREAT = TRAPPED_KINGS + 2;                    // the opponent has a capture available
constexpr int TEMPO = THREAT + 1;                            // our men's advancement minus theirs, -20..20
constexpr int OPPOSITION = TEMPO + 41;                       // [size bucket][men on board?][parity a][parity b]
constexpr int BIAS = OPPOSITION + 32;
constexpr int COUNT = BIAS + 1;

constexpr int MAX_ACTIVE = 128;

struct Active {
    int index;
    int value;
};

struct List {
    Active f[MAX_ACTIVE];
    int n = 0;
    int pieces = 0;  // total pieces on the board, for the phase blend
};

void extract(const Position& pos, List& out);

}  // namespace features

// Either a linear model (RGEV weights), an NNUE network (RGNN, see nnue.hpp) or the hand-crafted eval.
class Evaluator {
public:
    static constexpr int SCALE = 8;  // weights are stored in 1/8 centi-men

    // Built-in evaluation compiled into the binary: the embedded net if there is one, else the linear weights,
    // else the hand-crafted eval.
    static std::shared_ptr<const Evaluator> builtin();
    // RGEV or RGNN, told apart by the header; nullptr on failure (`error`, if given, says why).
    static std::shared_ptr<const Evaluator> from_file(const std::string& path, std::string* error = nullptr);
    static std::shared_ptr<const Evaluator> hce();

    Score operator()(const Position& pos) const;
    // The linear or hand-crafted eval, ignoring any network (the search's path when network() is null).
    Score linear(const Position& pos) const;
    bool trained() const { return !weights_.empty() || net_; }
    // The NNUE network, or nullptr for a linear or hand-crafted eval.
    const nnue::Network* network() const { return net_.get(); }

    // Interleaved [feature][phase] weights, phase 0 = middlegame, 1 = endgame.
    bool load(const unsigned char* data, std::size_t size);
    // RGEV weights or an RGNN network.
    bool load_any(const unsigned char* data, std::size_t size, std::string* error = nullptr);

private:
    std::vector<std::int16_t> weights_;
    std::shared_ptr<const nnue::Network> net_;
};

// Static evaluation with the built-in evaluator, from the side to move's point of view (a man is ~100).
Score evaluate(const Position& pos);

}  // namespace regicide
