// Move text for booklab: captures are written with their full jump path ("2x9x18"), so two capture routes with the
// same start and end squares (possible for men too, e.g. B:W6,7,14,15,32:B2, 2x9x18 and 2x11x18) never share a text.
// Move identity is (start, end, set of captured pieces), as in Move: equivalent routes that take the same pieces
// (e.g. 9x18x11x2x9 and 9x2x11x18x9 in B:W6,7,14,15,32:BK9) are the same move.
#pragma once

#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include "movegen.hpp"
#include "position.hpp"
#include "types.hpp"

namespace booklab {

inline std::string move_text(const regicide::Position& pos, const regicide::Move& m) {
    if (!m.is_capture()) return regicide::move_to_string(m);
    const std::vector<int> path = regicide::move_path(pos, m);
    std::string s;
    for (std::size_t i = 0; i < path.size(); ++i) s += (i ? "x" : "") + std::to_string(regicide::to_pdn(path[i]));
    return s;
}

namespace detail {

// PDN square 1..32 -> (row, column) on the 8x8 board; row 0 holds squares 1-4 on columns 1,3,5,7.
inline void rc(int n, int& r, int& c) {
    r = (n - 1) / 4;
    c = 2 * ((n - 1) % 4) + (r % 2 == 0 ? 1 : 0);
}

// Square jumped over between two PDN squares two diagonals apart, or 0.
inline int jumped(int a, int b) {
    int ra, ca, rb, cb;
    rc(a, ra, ca);
    rc(b, rb, cb);
    if (std::abs(ra - rb) != 2 || std::abs(ca - cb) != 2) return 0;
    const int r = (ra + rb) / 2, c = (ca + cb) / 2;
    return r * 4 + (c - (r % 2 == 0 ? 1 : 0)) / 2 + 1;
}

}  // namespace detail

// Number of legal moves the PDN text can denote. With only start and end squares: the single jump between them if
// they are one jump apart and that jump is legal (1); otherwise every legal move sharing them
// (more than one = ambiguous). With a longer path: the legal moves with that start, end and exactly the set of pieces
// the path jumps over (at most one, since legal moves are unique by start, end and captured set); 0 if the path is
// not a chain of jumps.
inline int matching_moves(const regicide::Position& pos, const std::string& text) {
    std::vector<int> sq;
    int n = -1;
    for (char ch : text + "-") {
        if (ch >= '0' && ch <= '9') n = (n < 0 ? 0 : n * 10) + (ch - '0');
        else if (n >= 0) sq.push_back(n), n = -1;
    }
    if (sq.size() < 2) return 0;
    std::set<int> caps;
    if (sq.size() > 2)
        for (std::size_t i = 0; i + 1 < sq.size(); ++i) {
            const int j = detail::jumped(sq[i], sq[i + 1]);
            if (!j || !caps.insert(j).second) return 0;  // not a jump, or the same piece jumped twice
        }
    // Two squares one jump apart name that single jump when it is legal (the exact reading wins, as in parse_move
    // and the referee since master PR #38); only otherwise are they an abbreviation of longer captures.
    const int single = sq.size() == 2 ? detail::jumped(sq[0], sq[1]) : 0;
    regicide::MoveList list;
    regicide::generate_moves(pos, list);
    int count = 0, exact = 0;
    for (const regicide::Move& m : list) {
        if (regicide::to_pdn(m.from) != sq.front() || regicide::to_pdn(m.to) != sq.back()) continue;
        std::set<int> mc;
        for (regicide::Bitboard b = m.captures; b; b &= b - 1) mc.insert(regicide::to_pdn(__builtin_ctzll(b)));
        if (sq.size() > 2 && mc != caps) continue;
        if (single && mc.size() == 1 && *mc.begin() == single) ++exact;
        ++count;
    }
    return exact ? 1 : count;
}

}  // namespace booklab
