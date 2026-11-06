#pragma once
// Self-contained board logic for the endgame database: forward move generation,
// predecessor ("unmove") generation and colour normalisation.
//
// Positions are kept in a *canonical* frame: the side to move always moves DOWN
// (toward row 0, like Black in types.hpp) and is crowned on row 0. A position with
// White to move is brought into this frame by rotating the board 180 degrees
// (bit reversal: square s -> 31 - s) and swapping colours.

#include <bit>
#include <cstdint>

#include "types.hpp"

namespace regicide::egdb::detail {

inline Bitboard rev(Bitboard b) {
#if defined(__clang__)
    return __builtin_bitreverse32(b);  // a single RBIT on arm64
#else
    b = ((b >> 1) & 0x55555555u) | ((b & 0x55555555u) << 1);
    b = ((b >> 2) & 0x33333333u) | ((b & 0x33333333u) << 2);
    b = ((b >> 4) & 0x0F0F0F0Fu) | ((b & 0x0F0F0F0Fu) << 4);
    return std::byteswap(b);
#endif
}

struct Pos {
    Bitboard sm = 0, sk = 0;  // side to move: men, kings (moves down)
    Bitboard om = 0, ok = 0;  // opponent: men, kings (moves up)

    Bitboard own() const { return sm | sk; }
    Bitboard opp() const { return om | ok; }
    Bitboard occ() const { return sm | sk | om | ok; }
    bool operator==(const Pos&) const = default;
};

// Swap roles and rotate 180 degrees; an involution. Used after every move.
inline Pos flip(const Pos& p) { return {rev(p.om), rev(p.ok), rev(p.sm), rev(p.sk)}; }

inline Pos from_colors(Bitboard black, Bitboard white, Bitboard kings, Color stm) {
    Pos p{black & ~kings, black & kings, white & ~kings, white & kings};
    return stm == BLACK ? p : flip(p);
}

// Leading-rank "advancement" of a side's men, 0..6 (0 = all men still on their own back row).
// Side to move moves down: advancement = 7 - lowest row. Opponent: highest row.
inline int adv_down(Bitboard men) { return (31 - std::countr_zero(men)) >> 2; }
inline int adv_up(Bitboard men) { return (31 - std::countl_zero(men)) >> 2; }

enum Dir { UL = 0, UR = 1, DL = 2, DR = 3 };

struct Tables {
    std::int8_t nb[32][4];  // neighbouring square in each direction, -1 if off board
    std::int8_t jp[32][4];  // landing square of a jump in each direction, -1 if off board
};

constexpr Tables make_tables() {
    Tables t{};
    Bitboard (*const dirs[4])(Bitboard) = {up_left, up_right, down_left, down_right};
    for (int sq = 0; sq < 32; ++sq)
        for (int d = 0; d < 4; ++d) {
            Bitboard n = dirs[d](bit(sq));
            Bitboard j = n ? dirs[d](n) : 0;
            t.nb[sq][d] = n ? std::int8_t(std::countr_zero(n)) : -1;
            t.jp[sq][d] = j ? std::int8_t(std::countr_zero(j)) : -1;
        }
    return t;
}
inline constexpr Tables TB = make_tables();

// Does the side to move have a capture? Bit-parallel over all pieces.
inline bool has_capture(const Pos& p) {
    const Bitboard opp = p.opp(), empty = ~p.occ(), down = p.sm | p.sk;
    if ((down_left(down_left(down) & opp) | down_right(down_right(down) & opp)) & empty) return true;
    if (p.sk && ((up_left(up_left(p.sk) & opp) | up_right(up_right(p.sk) & opp)) & empty)) return true;
    return false;
}

// Same question for the opponent (who moves up).
inline bool opp_has_capture(const Pos& p) {
    const Bitboard own = p.own(), empty = ~p.occ(), up = p.om | p.ok;
    if ((up_left(up_left(up) & own) | up_right(up_right(up) & own)) & empty) return true;
    if (p.ok && ((down_left(down_left(p.ok) & own) | down_right(down_right(p.ok) & own)) & empty)) return true;
    return false;
}

inline bool has_quiet_move(const Pos& p) {
    const Bitboard empty = ~p.occ(), down = p.sm | p.sk;
    if ((down_left(down) | down_right(down)) & empty) return true;
    return p.sk && ((up_left(p.sk) | up_right(p.sk)) & empty);
}

// Position after moving the piece on `from` to `to`, removing `captured`; returned in the canonical
// frame of the opponent, who is now to move.
inline Pos make_succ(const Pos& p, int from, int to, Bitboard captured, bool king_after) {
    Pos q = p;
    q.sm &= ~bit(from);
    q.sk &= ~bit(from);
    (king_after ? q.sk : q.sm) |= bit(to);
    q.om &= ~captured;
    q.ok &= ~captured;
    return flip(q);
}

namespace impl {
// Captured pieces stay on the board until the sequence is complete (they block and cannot be
// jumped twice). A man that reaches row 0 is crowned and its move ends.
template <class F>
bool jump_rec(const Pos& p, int from, int cur, bool king, Bitboard captured, Bitboard opp, Bitboard empty, F& f) {
    bool extended = false;
    for (int d = king ? 0 : 2; d < 4; ++d) {
        const int over = TB.nb[cur][d];
        if (over < 0 || !(opp & ~captured & bit(over))) continue;
        const int land = TB.jp[cur][d];
        if (land < 0 || !(empty & bit(land))) continue;
        extended = true;
        const Bitboard cap = captured | bit(over);
        if (!king && land < 4) {
            if (f(make_succ(p, from, land, cap, true))) return true;
        } else if (jump_rec(p, from, land, king, cap, opp, empty, f)) {
            return true;
        }
    }
    if (!extended && captured) return f(make_succ(p, from, cur, captured, king));
    return false;
}
}  // namespace impl

// Calls f(successor) for every capture sequence (successors may repeat). f returns true to stop.
// Returns true if enumeration was stopped by f.
template <class F>
bool for_each_capture(const Pos& p, F&& f) {
    const Bitboard opp = p.opp(), empty0 = ~p.occ();
    for (Bitboard b = p.sk; b; b &= b - 1) {
        const int sq = std::countr_zero(b);
        if (impl::jump_rec(p, sq, sq, true, 0, opp, empty0 | bit(sq), f)) return true;
    }
    for (Bitboard b = p.sm; b; b &= b - 1) {
        const int sq = std::countr_zero(b);
        if (impl::jump_rec(p, sq, sq, false, 0, opp, empty0 | bit(sq), f)) return true;
    }
    return false;
}

enum class Quiet : std::uint8_t { King, Man, Promotion };

// Calls f(successor, kind) for every non-capturing move. f returns true to stop.
template <class F>
bool for_each_quiet(const Pos& p, F&& f) {
    const Bitboard empty = ~p.occ();
    for (Bitboard b = p.sk; b; b &= b - 1) {
        const int sq = std::countr_zero(b);
        for (int d = 0; d < 4; ++d) {
            const int to = TB.nb[sq][d];
            if (to >= 0 && (empty & bit(to)) && f(make_succ(p, sq, to, 0, true), Quiet::King)) return true;
        }
    }
    for (Bitboard b = p.sm; b; b &= b - 1) {
        const int sq = std::countr_zero(b);
        for (int d = DL; d <= DR; ++d) {
            const int to = TB.nb[sq][d];
            if (to < 0 || !(empty & bit(to))) continue;
            const bool promo = to < 4;
            if (f(make_succ(p, sq, to, 0, promo), promo ? Quiet::Promotion : Quiet::Man)) return true;
        }
    }
    return false;
}

// Calls f(predecessor, man_unmove) for every position from which the opponent (the side that just
// moved) reaches p with a non-capturing, non-promoting move. Predecessors are returned canonical
// (the opponent to move). Un-promotions and un-captures are not generated: they belong to other
// slices, which look this position up instead.
template <class F>
void for_each_unmove(const Pos& p, F&& f) {
    const Bitboard empty = ~p.occ();
    for (Bitboard b = p.ok; b; b &= b - 1) {
        const int t = std::countr_zero(b);
        for (int d = 0; d < 4; ++d) {
            const int s = TB.nb[t][d];
            if (s < 0 || !(empty & bit(s))) continue;
            Pos q = p;
            q.ok ^= bit(t) | bit(s);
            f(flip(q), false);
        }
    }
    for (Bitboard b = p.om; b; b &= b - 1) {
        const int t = std::countr_zero(b);
        for (int d = DL; d <= DR; ++d) {  // opponent men move up, so they came from below
            const int s = TB.nb[t][d];
            if (s < 0 || !(empty & bit(s))) continue;
            Pos q = p;
            q.om ^= bit(t) | bit(s);
            f(flip(q), true);
        }
    }
}

}  // namespace regicide::egdb::detail
