#pragma once

#include "position.hpp"

namespace regicide {

using Score = int;

constexpr Score SCORE_INF = 32000;
constexpr Score SCORE_WIN = 30000;  // minus distance to the win, in plies

// Static evaluation from the side to move's point of view.
// Placeholder hand-crafted eval; this is the hook the NNUE will replace.
Score evaluate(const Position& pos);

}  // namespace regicide
