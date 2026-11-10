// Move text for booklab: captures are written with their full jump path ("2x9x18"), so two capture routes with the
// same start and end squares (possible for men too, e.g. B:W6,7,14,15,32:B2, 2x9x18 and 2x11x18) never share a text.
#pragma once

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

// Number of legal moves the PDN text can denote: with only start and end squares, every legal move sharing them;
// with a full path, the moves whose full path is exactly that text.
inline int matching_moves(const regicide::Position& pos, const std::string& text) {
    std::vector<int> sq;
    int n = -1;
    for (char ch : text + "-") {
        if (ch >= '0' && ch <= '9') n = (n < 0 ? 0 : n * 10) + (ch - '0');
        else if (n >= 0) sq.push_back(n), n = -1;
    }
    if (sq.size() < 2) return 0;
    regicide::MoveList list;
    regicide::generate_moves(pos, list);
    int count = 0;
    for (const regicide::Move& m : list) {
        if (regicide::to_pdn(m.from) != sq.front() || regicide::to_pdn(m.to) != sq.back()) continue;
        if (sq.size() == 2 || move_text(pos, m) == text) ++count;
    }
    return count;
}

}  // namespace booklab
