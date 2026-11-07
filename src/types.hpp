#pragma once

#include <array>
#include <bit>
#include <cstdint>

namespace regicide {

// 32 playable squares, 4 per row. Index 0 = a1, row 0 = rank 1.
// Even rows hold files a,c,e,g; odd rows hold files b,d,f,h.
//
//   row 7:   28  29  30  31      (b8 d8 f8 h8)
//   row 6: 24  25  26  27        (a7 c7 e7 g7)
//   ...
//   row 1:   4   5   6   7       (b2 d2 f2 h2)
//   row 0: 0   1   2   3         (a1 c1 e1 g1)
using Bitboard = std::uint32_t;
using Key = std::uint64_t;

enum Color : std::uint8_t { BLACK = 0, WHITE = 1 };  // Black moves first, from the top (row 7) down.

constexpr Color operator~(Color c) { return Color(c ^ 1); }

struct Move {
    std::uint8_t from = 0;
    std::uint8_t to = 0;
    Bitboard captures = 0;

    constexpr bool is_capture() const { return captures != 0; }
    constexpr bool is_null() const { return from == 0 && to == 0; }
    constexpr bool operator==(const Move&) const = default;
};

constexpr Bitboard bit(int sq) { return Bitboard(1) << sq; }
constexpr int popcount(Bitboard b) { return std::popcount(b); }
constexpr int lsb(Bitboard b) { return std::countr_zero(b); }
constexpr int pop_lsb(Bitboard& b) {
    int sq = lsb(b);
    b &= b - 1;
    return sq;
}
constexpr int row_of(int sq) { return sq >> 2; }

constexpr Bitboard ROW_0 = 0x0000000Fu;
constexpr Bitboard ROW_7 = 0xF0000000u;
constexpr Bitboard EVEN_ROWS = 0x0F0F0F0Fu;
constexpr Bitboard ODD_ROWS = 0xF0F0F0F0u;
constexpr Bitboard LEFT_EDGE = 0x01010101u;   // a-file squares (even rows, column 0)
constexpr Bitboard RIGHT_EDGE = 0x80808080u;  // h-file squares (odd rows, column 3)

constexpr Bitboard row_mask(int r) { return ROW_0 << (4 * r); }
constexpr Bitboard promo_row(Color c) { return c == WHITE ? ROW_7 : ROW_0; }
constexpr Bitboard back_row(Color c) { return c == WHITE ? ROW_0 : ROW_7; }
// Rows a man of colour c has advanced: 0 on its own back row, 6 one step from crowning.
constexpr int advance_of(Color c, int sq) { return c == WHITE ? row_of(sq) : 7 - row_of(sq); }

// Single-step diagonal shifts. "Up" means toward rank 8.
constexpr Bitboard up_left(Bitboard b) { return ((b & EVEN_ROWS & ~LEFT_EDGE) << 3) | ((b & ODD_ROWS) << 4); }
constexpr Bitboard up_right(Bitboard b) { return ((b & EVEN_ROWS) << 4) | ((b & ODD_ROWS & ~RIGHT_EDGE) << 5); }
constexpr Bitboard down_left(Bitboard b) { return ((b & EVEN_ROWS & ~LEFT_EDGE) >> 5) | ((b & ODD_ROWS) >> 4); }
constexpr Bitboard down_right(Bitboard b) { return ((b & EVEN_ROWS) >> 4) | ((b & ODD_ROWS & ~RIGHT_EDGE) >> 3); }

// Directions: 0 = up-left, 1 = up-right, 2 = down-left, 3 = down-right. opposite(d) = d ^ 3.
constexpr Bitboard shift(Bitboard b, int d) {
    switch (d) {
        case 0: return up_left(b);
        case 1: return up_right(b);
        case 2: return down_left(b);
        default: return down_right(b);
    }
}
constexpr int opposite(int d) { return d ^ 3; }

// Directions a piece may move in: white men go up (0,1), black men go down (2,3), kings go all four.
constexpr int dir_begin(Color c, bool king) { return king || c == WHITE ? 0 : 2; }
constexpr int dir_end(Color c, bool king) { return king || c == BLACK ? 4 : 2; }

// NEIGHBOR[sq][d] = adjacent square in direction d, or -1 off the board.
inline constexpr auto NEIGHBOR = [] {
    std::array<std::array<std::int8_t, 4>, 32> t{};
    for (int sq = 0; sq < 32; ++sq)
        for (int d = 0; d < 4; ++d) {
            Bitboard n = shift(bit(sq), d);
            t[sq][d] = n ? std::int8_t(std::countr_zero(n)) : std::int8_t(-1);
        }
    return t;
}();

// PDN numbering (1..32, square 1 = b8, square 32 = g1) <-> internal index.
constexpr int from_pdn(int n) { return (7 - (n - 1) / 4) * 4 + (n - 1) % 4; }
constexpr int to_pdn(int sq) { return (7 - sq / 4) * 4 + sq % 4 + 1; }

}  // namespace regicide
