#include "movegen.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace regicide {

namespace {

struct JumpCtx {
    MoveList& list;
    Bitboard opp;
    Bitboard empty;  // includes the moving piece's origin square
    Bitboard promo;
    int from;
    int dbegin, dend;
    bool king;
};

// Captured pieces stay on the board (they block and can't be jumped twice) until the move is complete.
void extend_jumps(JumpCtx& c, int cur, Bitboard captured) {
    bool extended = false;
    for (int d = c.dbegin; d < c.dend; ++d) {
        const int over = NEIGHBOR[cur][d];
        if (over < 0 || !(c.opp & ~captured & bit(over))) continue;
        const int land = NEIGHBOR[over][d];
        if (land < 0 || !(c.empty & bit(land))) continue;

        extended = true;
        const Bitboard caps = captured | bit(over);
        // A man that reaches the king row is crowned and its move ends there.
        if (!c.king && (bit(land) & c.promo))
            c.list.push({std::uint8_t(c.from), std::uint8_t(land), caps});
        else
            extend_jumps(c, land, caps);
    }
    if (!extended && captured) {
        const Move m{std::uint8_t(c.from), std::uint8_t(cur), captured};
        // Only kings can reach the same (to, captures) along different paths.
        if (!c.king || std::find(c.list.begin(), c.list.end(), m) == c.list.end()) c.list.push(m);
    }
}

}  // namespace

void generate_captures(const Position& pos, MoveList& list) {
    const Color us = pos.side_to_move();
    const Bitboard opp = pos.pieces(~us), empty = pos.empty();
    for (Bitboard b = capturers(pos, us); b;) {
        const int sq = pop_lsb(b);
        const bool king = pos.kings() & bit(sq);
        JumpCtx c{list, opp, empty | bit(sq), promo_row(us), sq, dir_begin(us, king), dir_end(us, king), king};
        extend_jumps(c, sq, 0);
    }
}

void generate_moves(const Position& pos, MoveList& list) {
    generate_captures(pos, list);
    if (!list.empty()) return;

    const Color us = pos.side_to_move();
    const Bitboard empty = pos.empty();
    for (int d = 0; d < 4; ++d) {
        const int o = opposite(d);
        for (Bitboard to = shift(movers(pos, us, d), d) & empty; to;) {
            const int t = pop_lsb(to);
            list.push({std::uint8_t(NEIGHBOR[t][o]), std::uint8_t(t), 0});
        }
    }
}

bool parse_move(const Position& pos, const std::string& text, Move& out) {
    std::vector<int> path;
    int n = -1;
    for (char ch : text + '-') {
        if (std::isdigit(static_cast<unsigned char>(ch))) {
            n = (n < 0 ? 0 : n * 10) + (ch - '0');
            if (n > 32) return false;  // also keeps a long run of digits from overflowing
        } else if (ch == '-' || ch == 'x' || ch == 'X' || ch == ':') {
            if (n < 1 || n > 32) return false;
            path.push_back(from_pdn(n));
            n = -1;
        } else {
            return false;
        }
    }
    if (path.size() < 2) return false;

    // Squares jumped over along the given path (only meaningful for complete capture paths).
    Bitboard jumped = 0;
    bool full_path = path.size() > 2;
    for (std::size_t i = 0; full_path && i + 1 < path.size(); ++i) {
        int mid = -1;
        for (int d = 0; d < 4; ++d) {
            int over = NEIGHBOR[path[i]][d];
            if (over >= 0 && NEIGHBOR[over][d] == path[i + 1]) mid = over;
        }
        // A path with more than two squares has to be a chain of jumps, each over another piece.
        if (mid < 0 || (jumped & bit(mid))) return false;
        jumped |= bit(mid);
    }

    // Two squares one jump apart can be that single jump or the ends of a longer capture.
    Bitboard single = 0;
    if (path.size() == 2)
        for (int d = 0; d < 4; ++d) {
            int over = NEIGHBOR[path[0]][d];
            if (over >= 0 && NEIGHBOR[over][d] == path[1]) single = bit(over);
        }

    MoveList list;
    generate_moves(pos, list);
    const Move* match = nullptr;
    for (const Move& m : list) {
        if (m.from != path.front() || m.to != path.back()) continue;
        if (full_path && m.captures != jumped) continue;
        if (!match || (single && m.captures == single)) match = &m;  // the exact reading wins
    }
    if (!match) return false;
    out = *match;
    return true;
}

namespace {

bool find_path(const Position& pos, const Move& m, int cur, Bitboard left, bool king, std::vector<int>& path) {
    if (!left) return cur == m.to;
    const Color us = pos.side_to_move();
    const Bitboard empty = pos.empty() | bit(m.from);
    for (int d = dir_begin(us, king); d < dir_end(us, king); ++d) {
        const int over = NEIGHBOR[cur][d];
        if (over < 0 || !(left & bit(over))) continue;
        const int land = NEIGHBOR[over][d];
        if (land < 0 || !(empty & bit(land))) continue;
        path.push_back(land);
        if (find_path(pos, m, land, left & ~bit(over), king, path)) return true;
        path.pop_back();
    }
    return false;
}

}  // namespace

std::string move_text(const Position& pos, const Move& m) {
    if (!m.is_capture()) return std::to_string(to_pdn(m.from)) + "-" + std::to_string(to_pdn(m.to));
    std::string s;
    for (int sq : move_path(pos, m)) s += (s.empty() ? "" : "x") + std::to_string(to_pdn(sq));
    return s;
}

std::vector<int> move_path(const Position& pos, const Move& m) {
    std::vector<int> path{m.from};
    if (!m.is_capture() || !find_path(pos, m, m.from, m.captures, pos.kings() & bit(m.from), path))
        path = {m.from, m.to};
    return path;
}

}  // namespace regicide
