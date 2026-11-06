#include "eval.hpp"

#include <algorithm>

#include "movegen.hpp"

namespace regicide {

namespace {

constexpr Score MAN = 100;
constexpr Score KING = 135;

// Indexed by rows advanced (0 = own back row, 6 = about to crown).
constexpr Score ADVANCE[7] = {0, 1, 3, 6, 10, 16, 24};

constexpr Bitboard CENTER = 0x00666600u;  // c3..f6 dark squares in the middle four rows
constexpr Bitboard SINGLE_CORNER = bit(0) | bit(31);                     // a1, h8
constexpr Bitboard DOUBLE_CORNER = bit(28) | bit(24) | bit(7) | bit(3);  // b8 a7, h2 g1

// Back-rank guards: for black (top) the key squares are b8/f8 ("bridge"), for white c1/g1.
constexpr Bitboard BRIDGE[2] = {bit(28) | bit(30), bit(1) | bit(3)};

// The forward cone of a man: an opposing piece outside it cannot get in its way before it crowns.
Bitboard cone_ahead(Color c, int sq) {
    Bitboard cone = 0, front = bit(sq);
    for (int r = advance_of(c, sq); r < 7; ++r) {
        front = c == WHITE ? (up_left(front) | up_right(front)) : (down_left(front) | down_right(front));
        cone |= front;
    }
    return cone;
}

Score side_score(const Position& pos, Color c, int total_pieces) {
    const Bitboard men = pos.men(c), kings = pos.kings(c);
    const Bitboard opp = pos.pieces(~c), opp_kings = pos.kings(~c);
    const int nmen = popcount(men), nkings = popcount(kings);

    Score s = nmen * MAN + nkings * KING;

    // Men: advancement grows in value as the board empties.
    const int adv_scale = 24 - total_pieces;  // 0 at the start, ~20 in endgames
    for (Bitboard b = men; b;) {
        const int sq = pop_lsb(b);
        const int a = advance_of(c, sq);
        s += ADVANCE[a] * (8 + adv_scale) / 16;
        // Runaway: no opposing piece can intercept before crowning.
        if (a >= 3 && !(cone_ahead(c, sq) & opp)) s += 30 + 6 * a;
    }

    s += 4 * popcount(men & CENTER);
    // Back rank: guarding the king row matters while the opponent has men to crown and no kings yet.
    if (!opp_kings) s += 10 * popcount(men & BRIDGE[c]) + 3 * popcount(men & back_row(c));

    // Kings: centralisation and mobility; corners are traps.
    for (Bitboard b = kings; b;) {
        const int sq = pop_lsb(b);
        s += 6 * popcount(CENTER & bit(sq));
    }
    s -= 12 * popcount(kings & SINGLE_CORNER);
    s -= 4 * popcount(kings & DOUBLE_CORNER);
    return s;
}

}  // namespace

Score evaluate(const Position& pos) {
    const Color us = pos.side_to_move();
    const int total = pos.piece_count();
    Score s = side_score(pos, us, total) - side_score(pos, ~us, total);

    // Mobility (quiet moves only).
    s += 2 * (quiet_move_count(pos, us) - quiet_move_count(pos, ~us));

    // Trade down when ahead: the same material edge is worth more with fewer pieces on the board.
    const int material = popcount(pos.pieces(us)) - popcount(pos.pieces(~us));
    s += material * (24 - total) * 3;

    return std::clamp(s, -SCORE_KNOWN_WIN + 1, SCORE_KNOWN_WIN - 1);
}

}  // namespace regicide
