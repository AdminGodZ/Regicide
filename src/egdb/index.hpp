#pragma once
// Perfect indexing of endgame positions (combinatorial number system).
//
// A *slice* is a material signature (a, b, c, d) = (side-to-move men, side-to-move kings,
// opponent men, opponent kings), always in the canonical frame of board.hpp. A slice is split
// into *subslices* (x, y): x = advancement of the side to move's leading man, y = advancement of
// the opponent's leading man (0 when a side has no men). A non-capturing man move never lowers
// its side's advancement, so subslices can be solved from the most advanced down.
//
// Index within a subslice:
//   ((ms * NO + mo) * NKS + ks) * NKO + ko
//   ms: colex rank of the side to move's men (mirrored so that they live on squares 0..27),
//       minus the number of configurations with lower advancement
//   mo: same for the opponent's men (squares 0..27 natively)
//   ks: colex rank of the side to move's kings among the squares not holding men
//   ko: colex rank of the opponent's kings among the remaining empty squares
// Men of both sides are indexed independently, so index space where they overlap is unused
// ("invalid"); those entries are don't-cares and compress to nothing.

#include <bit>
#include <cstdint>

#include "egdb/board.hpp"

namespace regicide::egdb::detail {

inline constexpr int MAX_PIECES = 8;  // total pieces supported by the index / file format
inline constexpr int MAX_SIDE = 7;    // pieces per side

struct BinomTable {
    std::uint64_t c[33][33];
};
constexpr BinomTable make_binom() {
    BinomTable t{};
    for (int n = 0; n <= 32; ++n) {
        t.c[n][0] = 1;
        for (int k = 1; k <= n; ++k) t.c[n][k] = t.c[n - 1][k - 1] + (k <= n - 1 ? t.c[n - 1][k] : 0);
    }
    return t;
}
inline constexpr BinomTable BN = make_binom();
constexpr std::uint64_t binom(int n, int k) { return (n < 0 || k < 0 || k > n) ? 0 : BN.c[n][k]; }

// Colex rank of a set of squares: sum over the i-th smallest element s_i of C(s_i, i+1).
inline std::uint32_t colex(Bitboard m) {
    std::uint32_t r = 0;
    for (int i = 1; m; ++i, m &= m - 1) r += std::uint32_t(BN.c[std::countr_zero(m)][i]);
    return r;
}

// Colex rank of `set` after compressing it onto the squares of `avail` (set must be a subset).
inline std::uint32_t colex_in(Bitboard set, Bitboard avail) {
    std::uint32_t r = 0;
    for (int i = 1; set; ++i, set &= set - 1) {
        const Bitboard below = (set & -set) - 1;
        r += std::uint32_t(BN.c[std::popcount(avail & below)][i]);
    }
    return r;
}

// Men configurations with leading row exactly r (on squares 0..27): [men_base, men_base + men_count).
constexpr std::uint64_t men_base(int n, int r) { return n ? binom(4 * r, n) : 0; }
constexpr std::uint64_t men_count(int n, int r) { return n ? binom(4 * r + 4, n) - binom(4 * r, n) : (r == 0); }

struct Material {
    int a = 0, b = 0, c = 0, d = 0;
    int total() const { return a + b + c + d; }
    Material partner() const { return {c, d, a, b}; }
    bool operator==(const Material&) const = default;
};

inline Material material_of(const Pos& p) {
    return {std::popcount(p.sm), std::popcount(p.sk), std::popcount(p.om), std::popcount(p.ok)};
}

struct SubDims {
    Material m;
    int x = 0, y = 0;
    std::uint64_t ns = 0, no = 0, nks = 0, nko = 0;
    std::uint64_t size = 0;
};

inline SubDims make_dims(Material m, int x, int y) {
    SubDims s;
    s.m = m;
    s.x = x;
    s.y = y;
    s.ns = men_count(m.a, x);
    s.no = men_count(m.c, y);
    s.nks = binom(32 - m.a - m.c, m.b);
    s.nko = binom(32 - m.a - m.c - m.b, m.d);
    s.size = s.ns * s.no * s.nks * s.nko;
    return s;
}

inline int adv_x(const Pos& p) { return p.sm ? adv_down(p.sm) : 0; }
inline int adv_y(const Pos& p) { return p.om ? adv_up(p.om) : 0; }

// Index of p within its own subslice (whose dimensions must be `s`).
inline std::uint64_t index_of(const SubDims& s, const Pos& p) {
    const Bitboard rs = rev(p.sm);
    const std::uint64_t ms = colex(rs) - men_base(s.m.a, s.x);
    const std::uint64_t mo = colex(p.om) - men_base(s.m.c, s.y);
    const Bitboard free0 = ~(p.sm | p.om);
    const std::uint64_t ks = colex_in(p.sk, free0);
    const std::uint64_t ko = colex_in(p.ok, free0 & ~p.sk);
    return ((ms * s.no + mo) * s.nks + ks) * s.nko + ko;
}

// Tables of all k-subsets of {0..31} in colex order (k <= 7), built lazily. Generator-side only.
const std::uint32_t* unrank_table(int k);

// k-th (0-based) set bit of v.
inline int select_bit(Bitboard v, int k) {
    int pos = 0;
    int c = std::popcount(v & 0xFFFFu);
    if (k >= c) k -= c, v >>= 16, pos += 16;
    c = std::popcount(v & 0xFFu);
    if (k >= c) k -= c, v >>= 8, pos += 8;
    for (;; v &= v - 1)
        if (k-- == 0) return pos + std::countr_zero(v);
}

// Scatter the low bits of `mask` onto the set squares of `avail` (software PDEP).
inline Bitboard scatter(Bitboard mask, Bitboard avail) {
    Bitboard r = 0;
    for (; mask; mask &= mask - 1) r |= bit(select_bit(avail, std::countr_zero(mask)));
    return r;
}

// Inverse of index_of. Returns false for index entries where the two sides' men overlap.
inline bool unindex(const SubDims& s, std::uint64_t idx, Pos& p) {
    const std::uint64_t ko = idx % s.nko;
    idx /= s.nko;
    const std::uint64_t ks = idx % s.nks;
    idx /= s.nks;
    const std::uint64_t mo = idx % s.no;
    const std::uint64_t ms = idx / s.no;
    p.sm = rev(unrank_table(s.m.a)[ms + men_base(s.m.a, s.x)]);
    p.om = unrank_table(s.m.c)[mo + men_base(s.m.c, s.y)];
    if (p.sm & p.om) return false;
    const Bitboard free0 = ~(p.sm | p.om);
    p.sk = scatter(unrank_table(s.m.b)[ks], free0);
    p.ok = scatter(unrank_table(s.m.d)[ko], free0 & ~p.sk);
    return true;
}

}  // namespace regicide::egdb::detail
