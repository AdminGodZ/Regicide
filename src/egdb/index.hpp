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

// Tables of all k-subsets of {0..31} in colex order (k <= 7), built on first use (~18 MB).
// Generator / verification side only; probing never unranks.
const std::uint32_t* const* unrank_tables();
inline const std::uint32_t* unrank_table(int k) {
    static const std::uint32_t* const* t = unrank_tables();
    return t[k];
}

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

// Sequential walk over a subslice in index order, avoiding the divisions and bit selects of
// unindex(). After set(i) / next(), `valid` tells whether the entry is a real position.
class Cursor {
public:
    explicit Cursor(const SubDims& d)
        : d_(d),
          us_(unrank_table(d.m.a) + men_base(d.m.a, d.x)),
          uo_(unrank_table(d.m.c) + men_base(d.m.c, d.y)),
          ub_(unrank_table(d.m.b)),
          ud_(unrank_table(d.m.d)) {}

    Pos p;
    bool valid = false;

    void set(std::uint64_t idx) {
        ko_ = idx % d_.nko;
        idx /= d_.nko;
        ks_ = idx % d_.nks;
        idx /= d_.nks;
        mo_ = idx % d_.no;
        ms_ = idx / d_.no;
        men();
    }

    void next() {
        if (++ko_ < d_.nko) {
            if (valid) p.ok = place(ud_[ko_], free2_);
            return;
        }
        ko_ = 0;
        if (++ks_ < d_.nks) {
            if (valid) kings();
            return;
        }
        ks_ = 0;
        if (++mo_ == d_.no) mo_ = 0, ++ms_;
        men();
    }

private:
    static Bitboard place(Bitboard mask, const std::uint8_t* sq) {
        Bitboard r = 0;
        for (; mask; mask &= mask - 1) r |= bit(sq[std::countr_zero(mask)]);
        return r;
    }
    static void list(Bitboard avail, std::uint8_t* out) {
        for (int i = 0; avail; avail &= avail - 1) out[i++] = std::uint8_t(std::countr_zero(avail));
    }
    void kings() {
        p.sk = place(ub_[ks_], free1_);
        list(free0_ & ~p.sk, free2_);
        p.ok = place(ud_[ko_], free2_);
    }
    void men() {
        p.sm = rev(us_[ms_]);
        p.om = uo_[mo_];
        valid = !(p.sm & p.om);
        if (!valid) return;
        free0_ = ~(p.sm | p.om);
        list(free0_, free1_);
        kings();
    }

    const SubDims& d_;
    const std::uint32_t *us_, *uo_, *ub_, *ud_;
    std::uint64_t ms_ = 0, mo_ = 0, ks_ = 0, ko_ = 0;
    Bitboard free0_ = 0;
    std::uint8_t free1_[32], free2_[32];
};

}  // namespace regicide::egdb::detail
