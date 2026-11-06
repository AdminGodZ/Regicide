#pragma once

#include "position.hpp"

namespace regicide {

using Score = int;

constexpr int MAX_PLY = 128;

constexpr Score SCORE_INF = 32000;
constexpr Score SCORE_WIN = 30000;                    // minus distance to the win, in plies
constexpr Score SCORE_MATE_BOUND = SCORE_WIN - 1000;  // above: a forced win by running out of moves
constexpr Score SCORE_DB_WIN = 20000;                 // endgame-database win, minus distance in plies
constexpr Score SCORE_KNOWN_WIN = 15000;              // above: proven win (mate or database)
constexpr Score SCORE_DRAW = 0;

constexpr bool is_decisive(Score s) { return s >= SCORE_KNOWN_WIN || s <= -SCORE_KNOWN_WIN; }

// Static evaluation from the side to move's point of view, in centi-men (a man is ~100).
Score evaluate(const Position& pos);

}  // namespace regicide
