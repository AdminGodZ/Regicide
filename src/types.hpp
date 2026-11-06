#pragma once

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

enum Color : std::uint8_t { BLACK = 0, WHITE = 1 };  // Black moves first, from the top (row 7) down.

constexpr Color operator~(Color c) { return Color(c ^ 1); }

struct Move {
    std::uint8_t from = 0;
    std::uint8_t to = 0;
    Bitboard captures = 0;

    bool is_capture() const { return captures != 0; }
    bool operator==(const Move&) const = default;
};

constexpr Bitboard bit(int sq) { return Bitboard(1) << sq; }
inline int popcount(Bitboard b) { return std::popcount(b); }
inline int lsb(Bitboard b) { return std::countr_zero(b); }
inline int pop_lsb(Bitboard& b) {
    int sq = lsb(b);
    b &= b - 1;
    return sq;
}

constexpr Bitboard ROW_0 = 0x0000000Fu;
constexpr Bitboard ROW_7 = 0xF0000000u;
constexpr Bitboard EVEN_ROWS = 0x0F0F0F0Fu;
constexpr Bitboard ODD_ROWS = 0xF0F0F0F0u;
constexpr Bitboard LEFT_EDGE = 0x01010101u;   // a-file squares (even rows, column 0)
constexpr Bitboard RIGHT_EDGE = 0x80808080u;  // h-file squares (odd rows, column 3)

// Single-step diagonal shifts. "Up" means toward rank 8.
constexpr Bitboard up_left(Bitboard b) { return ((b & EVEN_ROWS & ~LEFT_EDGE) << 3) | ((b & ODD_ROWS) << 4); }
constexpr Bitboard up_right(Bitboard b) { return ((b & EVEN_ROWS) << 4) | ((b & ODD_ROWS & ~RIGHT_EDGE) << 5); }
constexpr Bitboard down_left(Bitboard b) { return ((b & EVEN_ROWS & ~LEFT_EDGE) >> 5) | ((b & ODD_ROWS) >> 4); }
constexpr Bitboard down_right(Bitboard b) { return ((b & EVEN_ROWS) >> 4) | ((b & ODD_ROWS & ~RIGHT_EDGE) >> 3); }

// PDN numbering (1..32, square 1 = b8, square 32 = g1) <-> internal index.
constexpr int from_pdn(int n) { return (7 - (n - 1) / 4) * 4 + (n - 1) % 4; }
constexpr int to_pdn(int sq) { return (7 - sq / 4) * 4 + sq % 4 + 1; }

}  // namespace regicide
