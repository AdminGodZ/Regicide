#include "movegen.hpp"

#include <algorithm>

namespace regicide {

namespace {

using ShiftFn = Bitboard (*)(Bitboard);
constexpr ShiftFn DIRS[4] = {up_left, up_right, down_left, down_right};

// Directions a piece may move in: white men go up, black men go down, kings go both ways.
constexpr int dir_begin(Color c, bool king) { return king || c == WHITE ? 0 : 2; }
constexpr int dir_end(Color c, bool king) { return king || c == BLACK ? 4 : 2; }

void add_unique(MoveList& list, const Move& m) {
    if (std::find(list.begin(), list.end(), m) == list.end()) list.push(m);
}

// Captured pieces stay on the board (they block and can't be jumped twice) until the move is complete.
void extend_jumps(MoveList& list, Color us, bool king, int from, Bitboard cur, Bitboard captured,
                  Bitboard opp, Bitboard empty) {
    const Bitboard promo_row = us == WHITE ? ROW_7 : ROW_0;
    bool extended = false;

    for (int d = dir_begin(us, king); d < dir_end(us, king); ++d) {
        Bitboard over = DIRS[d](cur) & opp & ~captured;
        if (!over) continue;
        Bitboard land = DIRS[d](over) & empty;
        if (!land) continue;

        extended = true;
        // A man that reaches the king row is crowned and its move ends there.
        if (!king && (land & promo_row))
            add_unique(list, {std::uint8_t(from), std::uint8_t(lsb(land)), captured | over});
        else
            extend_jumps(list, us, king, from, land, captured | over, opp, empty);
    }

    if (!extended && captured) add_unique(list, {std::uint8_t(from), std::uint8_t(lsb(cur)), captured});
}

}  // namespace

void generate_moves(const Position& pos, MoveList& list) {
    const Color us = pos.side_to_move();
    const Bitboard own = pos.pieces(us), opp = pos.pieces(~us), empty = pos.empty();

    for (Bitboard b = own; b;) {
        int sq = pop_lsb(b);
        bool king = pos.kings() & bit(sq);
        extend_jumps(list, us, king, sq, bit(sq), 0, opp, empty | bit(sq));
    }
    if (!list.empty()) return;

    for (Bitboard b = own; b;) {
        int sq = pop_lsb(b);
        bool king = pos.kings() & bit(sq);
        for (int d = dir_begin(us, king); d < dir_end(us, king); ++d)
            if (Bitboard to = DIRS[d](bit(sq)) & empty) list.push({std::uint8_t(sq), std::uint8_t(lsb(to)), 0});
    }
}

}  // namespace regicide
