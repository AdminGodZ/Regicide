#include "eval.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>
#include <iterator>

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

Score evaluate_hce(const Position& pos) {
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

namespace features {

namespace {

// Maps 8 binary digits to the same digits in base 3.
constexpr auto BASE3 = [] {
    std::array<std::uint16_t, 256> t{};
    for (int i = 0; i < 256; ++i) {
        int v = 0, p = 1;
        for (int b = 0; b < 8; ++b, p *= 3) v += ((i >> b) & 1) * p;
        t[i] = std::uint16_t(v);
    }
    return t;
}();

// A 4x4 window covers 4 rows and holds 2 adjacent dark squares in each: indices 4r + k and 4r + k + 1.
struct Window {
    int row0;
    int k[4];
};

constexpr auto WINDOW_DEFS = [] {
    std::array<Window, WINDOWS> w{};
    int n = 0;
    for (int r0 = 0; r0 <= 4; ++r0)
        for (int c = 0; c <= 4; ++c) {
            w[n].row0 = r0;
            for (int j = 0; j < 4; ++j) {
                const int r = r0 + j;
                // Even rows hold files 0,2,4,6 and odd rows 1,3,5,7: take the first dark file >= c.
                const int first = (r % 2 == 0) ? c + (c & 1) : c + 1 - (c & 1);
                w[n].k[j] = (r % 2 == 0) ? first / 2 : (first - 1) / 2;
            }
            ++n;
        }
    return w;
}();

inline unsigned gather(Bitboard b, const Window& w) {
    unsigned r = 0;
    for (int j = 0; j < 4; ++j) r |= ((b >> (4 * (w.row0 + j) + w.k[j])) & 3u) << (2 * j);
    return r;
}

// Oriented board: "us" moves up (toward row 7), "them" moves down.
struct Oriented {
    Bitboard us, them, kings;
};

Oriented orient(const Position& pos) {
    const Color stm = pos.side_to_move();
    Bitboard us = pos.pieces(stm), them = pos.pieces(~stm), kings = pos.kings();
    if (stm == BLACK) {
        us = __builtin_bitreverse32(us);
        them = __builtin_bitreverse32(them);
        kings = __builtin_bitreverse32(kings);
    }
    return {us, them, kings};
}

int quiet_moves(Bitboard men, Bitboard kings, Bitboard empty, bool up) {
    int n = 0;
    if (up) {
        n += popcount(up_left(men | kings) & empty) + popcount(up_right(men | kings) & empty);
        n += popcount(down_left(kings) & empty) + popcount(down_right(kings) & empty);
    } else {
        n += popcount(down_left(men | kings) & empty) + popcount(down_right(men | kings) & empty);
        n += popcount(up_left(kings) & empty) + popcount(up_right(kings) & empty);
    }
    return n;
}

bool can_capture_down(Bitboard men, Bitboard kings, Bitboard opp, Bitboard empty) {
    // Pieces moving down (men and kings), plus kings moving up.
    const Bitboard down = men | kings;
    return (down_left(down_left(down) & opp) & empty) || (down_right(down_right(down) & opp) & empty) ||
           (up_left(up_left(kings) & opp) & empty) || (up_right(up_right(kings) & opp) & empty);
}

constexpr auto CONE_UP = [] {
    std::array<Bitboard, 32> t{};
    for (int sq = 0; sq < 32; ++sq) {
        Bitboard c = 0, f = bit(sq);
        for (int r = sq / 4; r < 7; ++r) {
            f = up_left(f) | up_right(f);
            c |= f;
        }
        t[sq] = c;
    }
    return t;
}();

constexpr auto CONE_DOWN = [] {
    std::array<Bitboard, 32> t{};
    for (int sq = 0; sq < 32; ++sq) {
        Bitboard c = 0, f = bit(sq);
        for (int r = sq / 4; r > 0; --r) {
            f = down_left(f) | down_right(f);
            c |= f;
        }
        t[sq] = c;
    }
    return t;
}();

}  // namespace

// Calls f(index, value) for every active feature; returns the number of pieces on the board.
template <class F>
inline int visit(const Position& pos, F&& f) {
    const Oriented o = orient(pos);
    const Bitboard empty = ~(o.us | o.them);
    const Bitboard our_kings = o.us & o.kings, their_kings = o.them & o.kings;
    const Bitboard our_men = o.us & ~o.kings, their_men = o.them & ~o.kings;

    const int window_base = PATTERN + (WINDOW_SETS > 1 && o.kings ? WINDOWS * WINDOW_STATES : 0);
    for (int w = 0; w < WINDOWS; ++w) {
        const Window& win = WINDOW_DEFS[w];
        f(window_base + w * WINDOW_STATES + BASE3[gather(o.us, win)] + 2 * BASE3[gather(o.them, win)], 1);
    }
    for (int r = 0; r < BANDS; ++r)
        f(BAND + r * WINDOW_STATES + BASE3[(o.us >> (4 * r)) & 0xFF] + 2 * BASE3[(o.them >> (4 * r)) & 0xFF], 1);

    const int om = popcount(our_men), ok = popcount(our_kings), tm = popcount(their_men), tk = popcount(their_kings);
    f(MATERIAL + 0, om);
    f(MATERIAL + 1, ok);
    f(MATERIAL + 2, tm);
    f(MATERIAL + 3, tk);
    f(MATERIAL_TABLE + ((std::min(om, 12) * 13 + std::min(ok, 12)) * 13 + std::min(tm, 12)) * 13 + std::min(tk, 12), 1);

    for (Bitboard b = our_kings; b;) f(KING_SQ + pop_lsb(b), 1);
    for (Bitboard b = their_kings; b;) f(KING_SQ + 32 + pop_lsb(b), 1);

    f(MOBILITY + std::min(quiet_moves(our_men, our_kings, empty, true), 23), 1);
    f(MOBILITY + 24 + std::min(quiet_moves(their_men, their_kings, empty, false), 23), 1);

    for (Bitboard b = our_men; b;) {
        const int sq = pop_lsb(b);
        if (!(CONE_UP[sq] & o.them)) f(RUNAWAY + (7 - row_of(sq)), 1);
    }
    for (Bitboard b = their_men; b;) {
        const int sq = pop_lsb(b);
        if (!(CONE_DOWN[sq] & o.us)) f(RUNAWAY + 7 + row_of(sq), 1);
    }

    auto trapped = [&](Bitboard kings) {
        const Bitboard free = up_left(empty) | up_right(empty) | down_left(empty) | down_right(empty);
        return popcount(kings & ~free);
    };
    if (const int t = trapped(our_kings)) f(TRAPPED_KINGS + 0, t);
    if (const int t = trapped(their_kings)) f(TRAPPED_KINGS + 1, t);

    if (can_capture_down(their_men, their_kings, o.us, empty)) f(THREAT, 1);

    // Tempo: rows advanced by our men (moving up) minus rows advanced by theirs (moving down).
    int tempo = 0;
    for (Bitboard b = our_men; b;) tempo += row_of(pop_lsb(b));
    for (Bitboard b = their_men; b;) tempo -= 7 - row_of(pop_lsb(b));
    f(TEMPO + std::clamp(tempo, -20, 20) + 20, 1);

    // "The move" (opposition): parities of the pieces on the two halves of the board's square colouring, which
    // decide who must give way in sparse endgames. Bucketed by size; the model learns what matters.
    const Bitboard all = o.us | o.them;
    const int n = popcount(all);
    const int bucket = n <= 4 ? 0 : n <= 6 ? 1 : n <= 8 ? 2 : 3;
    const int pa = popcount(all & EVEN_ROWS) & 1, pb = popcount(all & 0x33333333u) & 1;
    f(OPPOSITION + ((bucket * 2 + (our_men | their_men ? 1 : 0)) * 2 + pa) * 2 + pb, 1);

    f(BIAS, 1);
    return popcount(o.us | o.them);
}

void extract(const Position& pos, List& out) {
    out.n = 0;
    out.pieces = visit(pos, [&](int index, int value) { out.f[out.n++] = {index, value}; });
}

}  // namespace features

bool Evaluator::load(const unsigned char* data, std::size_t size) {
    constexpr std::size_t header = 12;
    const std::size_t need = header + std::size_t(features::COUNT) * 2 * sizeof(std::int16_t);
    if (size < need || std::memcmp(data, "RGEV", 4) != 0) return false;
    std::uint32_t version, count;
    std::memcpy(&version, data + 4, 4);
    std::memcpy(&count, data + 8, 4);
    if (version != 1 || count != std::uint32_t(features::COUNT)) return false;
    weights_.resize(std::size_t(features::COUNT) * 2);
    std::memcpy(weights_.data(), data + header, weights_.size() * sizeof(std::int16_t));
    return true;
}

Score Evaluator::operator()(const Position& pos) const {
    if (weights_.empty()) return evaluate_hce(pos);
    const std::int16_t* w = weights_.data();
    int mg = 0, eg = 0;
    const int pieces = features::visit(pos, [&](int index, int value) {
        mg += w[2 * index] * value;
        eg += w[2 * index + 1] * value;
    });
    const int phase = std::min(pieces, 24);
    const Score s = (mg * phase + eg * (24 - phase)) / (24 * SCALE);
    return std::clamp(s, -SCORE_KNOWN_WIN + 1, SCORE_KNOWN_WIN - 1);
}

namespace {

// A leading zero keeps the array valid when the weights file is empty (no trained eval yet).
constexpr unsigned char EMBEDDED[] = {
    0,
#embed "weights/eval.bin"
};

}  // namespace

std::shared_ptr<const Evaluator> Evaluator::builtin() {
    static const std::shared_ptr<const Evaluator> e = [] {
        auto ev = std::make_shared<Evaluator>();
        ev->load(EMBEDDED + 1, sizeof(EMBEDDED) - 1);
        return ev;
    }();
    return e;
}

std::shared_ptr<const Evaluator> Evaluator::hce() {
    static const auto e = std::make_shared<const Evaluator>();
    return e;
}

std::shared_ptr<const Evaluator> Evaluator::from_file(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) return nullptr;
    std::vector<unsigned char> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    auto ev = std::make_shared<Evaluator>();
    if (!ev->load(data.data(), data.size())) return nullptr;
    return ev;
}

Score evaluate(const Position& pos) {
    static const Evaluator& e = *Evaluator::builtin();
    return e(pos);
}

}  // namespace regicide
