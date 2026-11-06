#pragma once
// Independent checks of a loaded database: one-ply minimax consistency and position counts.

#include <cstdint>
#include <string>

#include "egdb/index.hpp"

namespace regicide::egdb::detail {

struct VerifyStats {
    std::uint64_t checked = 0;       // non-capture positions compared
    std::uint64_t captures = 0;      // capture positions met (resolved by search, not stored)
    std::uint64_t wld_bad = 0;
    std::uint64_t mtc_checked = 0;
    std::uint64_t mtc_bad = 0;
    std::uint64_t counts[3] = {};    // win, draw, loss among checked
    std::string first_error;

    void merge(const VerifyStats& o);
};

// For `samples` random positions of slice m (every position if samples == 0), checks that the
// stored value equals the minimax of the successors' values (no legal move = loss), and, if
// check_mtc, that the stored moves-to-conversion satisfies its recurrence.
VerifyStats verify_slice(Material m, std::uint64_t samples, std::uint64_t seed, int threads, bool check_mtc);

// Number of piece placements with exactly n pieces (either side may be empty, no side to move);
// the quantity tabulated by Schaeffer et al. (e.g. 2,503,611,964 for n = 6).
std::uint64_t count_positions_formula(int n);
// The same number obtained by enumerating the index (valid entries of all slices) plus the
// placements where one side has no pieces.
std::uint64_t count_positions_index(int n);

std::string describe(const Pos& p);  // FEN-like, PDN squares, side to move = "B" in canonical frame

}  // namespace regicide::egdb::detail
