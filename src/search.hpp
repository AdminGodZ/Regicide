#pragma once

#include <cstdint>

#include "eval.hpp"
#include "position.hpp"

namespace regicide {

struct SearchResult {
    Move best;
    Score score = 0;
    int depth = 0;
    std::uint64_t nodes = 0;
};

// Iterative-deepening alpha-beta. Prints one info line per completed depth.
SearchResult search(const Position& pos, int max_depth);

}  // namespace regicide
