#pragma once
// Retrograde generator for the endgame database.

#include <cstdint>
#include <string>
#include <vector>

#include "egdb/index.hpp"
#include "egdb/platform.hpp"

namespace regicide::egdb::detail {

struct BuildOptions {
    std::string dir;       // output directory (part files go to dir/parts)
    int threads = 8;
    int mtc_pieces = 0;    // also build moves-to-conversion tables for slices with <= this many pieces
    bool verbose = true;   // log per-subslice progress
    // Optional filter for the largest piece count: only build slices whose per-side piece counts
    // {a+b, c+d} are listed (e.g. {4,4} and {5,3}). Such a family depends only on itself and on
    // smaller piece counts, so the most valuable 8-piece families can be built first.
    std::vector<std::pair<int, int>> sides;
};

// All materials with n pieces, at least one per side and at most MAX_SIDE per side.
std::vector<Material> materials_with(int n);

// Builds every slice with min_pieces..max_pieces pieces whose files are missing in opt.dir,
// smallest first. Already complete slices (and finished parts of a slice) are reused.
bool build_range(const BuildOptions& opt, int min_pieces, int max_pieces);

// Builds the slice pair {m, m.partner()} (all smaller slices must be loaded).
bool build_pair(const BuildOptions& opt, Material m);

void log_line(const char* fmt, ...) EGDB_PRINTF(1, 2);

}  // namespace regicide::egdb::detail
