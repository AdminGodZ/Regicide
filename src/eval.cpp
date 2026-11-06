#include "eval.hpp"

namespace regicide {

namespace {

constexpr Score MAN_VALUE = 100;
constexpr Score KING_VALUE = 130;
constexpr Score ADVANCE_BONUS = 2;  // per row a man has advanced
constexpr Score BACK_RANK_BONUS = 8;  // men guarding their own king row

Score side_score(const Position& pos, Color c) {
    const Bitboard own = pos.pieces(c);
    const Bitboard men = own & ~pos.kings();
    const Bitboard kings = own & pos.kings();

    Score s = popcount(men) * MAN_VALUE + popcount(kings) * KING_VALUE;

    for (Bitboard b = men; b;) {
        int row = pop_lsb(b) / 4;
        s += ADVANCE_BONUS * (c == WHITE ? row : 7 - row);
    }
    s += BACK_RANK_BONUS * popcount(men & (c == WHITE ? ROW_0 : ROW_7));
    return s;
}

}  // namespace

Score evaluate(const Position& pos) {
    const Color us = pos.side_to_move();
    return side_score(pos, us) - side_score(pos, ~us);
}

}  // namespace regicide
