#pragma once

#include <cstdint>

#include "position.hpp"

namespace regicide {

std::uint64_t perft(const Position& pos, int depth);

// Prints the node count below each root move, then the total.
std::uint64_t perft_divide(const Position& pos, int depth);

}  // namespace regicide
