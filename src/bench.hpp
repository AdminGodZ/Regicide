#pragma once

#include <vector>

#include "position.hpp"
#include "search.hpp"

namespace regicide {

// Deterministic benchmark positions: the start position plus positions reached by seeded random playouts.
std::vector<Position> bench_positions();

// Searches every bench position to a fixed depth (0 = default) and prints total nodes and speed.
// The node count is a signature: it changes only when search behaviour changes.
void run_bench(Engine& engine, int depth);

}  // namespace regicide
