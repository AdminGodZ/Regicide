#pragma once

// NNUE evaluation: a small quantised network over the pieces, seen from a colour's point of view oriented like the
// linear eval (that colour goes "up"; black's view is rotated 180 degrees with colours swapped). Two architectures,
// told apart by the version in the file header; the layer widths come from the header too, so one build loads any
// net within the limits below.
//
//   inputs   128 = [our man, our king, their man, their king] x 32 squares
//   FT       128 -> FT per point of view, int16 weights, SCReLU: a = clamp(acc, 0, QA)^2 >> 9 (0..127, 1.0 = 127)
//   L1       N -> L1, int8 weights, int32 sums, clipped ReLU: h = clamp(sum >> 6, 0, 127)
//   L2       L1 -> L2, same
//   output   L2 -> 1, int16 weights; score = out * out_scale / (127 * 64)
//
//   version 1: N = FT, the side to move's FT output. Only the output layer has buckets (by piece count).
//   version 2: N = 2 FT, the side to move's FT output followed by the opponent's (the FT weights are shared by the
//              two points of view). L1, L2 and the output layer all have one set of weights per bucket.
//
// The search keeps one accumulator per colour's point of view on its stack and updates them from each move; version
// 1 only needs the side to move's. tools/nnue/ trains and exports nets (see its README).
//
// File format (little endian): "RGNN", then u32 version, inputs, ft, l1, l2, buckets, activation, qa, qb,
// out_scale, then ft_bias i16[FT], ft_weight i16[INPUTS][FT], l1_bias i32[HB][L1], l1_weight i8[HB][L1][N],
// l2_bias i32[HB][L2], l2_weight i8[HB][L2][L1], out_bias i32[BUCKETS], out_weight i16[BUCKETS][L2], where HB (hidden
// buckets) is 1 in version 1 and BUCKETS in version 2. Inputs, activation, qa and qb must equal the constants
// below, and the widths must be ones this build supports, or the net is rejected with the reason.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <memory>
#include <new>
#include <string>
#include <vector>

#include "eval.hpp"

namespace regicide::nnue {

// The widest feature transformer (per point of view) this build loads: it sets the size of the search's
// accumulators. Raise it at configure time (cmake -DREGICIDE_NNUE_MAX_FT=2048) for wider nets.
#ifndef REGICIDE_NNUE_MAX_FT
#define REGICIDE_NNUE_MAX_FT 1024
#endif

constexpr int INPUTS = 128;
constexpr int MAX_FT = REGICIDE_NNUE_MAX_FT;
constexpr int FT_TILE = 128;  // FT widths are multiples of this (see Network::update)
constexpr int MAX_HIDDEN = 64;
constexpr int MAX_BUCKETS = 24;

constexpr int ACTIVATION = 1;  // 1 = SCReLU on the feature transformer (recorded in the header)
constexpr int QA = 255;        // feature transformer: 1.0 = 255
constexpr int QB = 64;         // hidden and output weights: 1.0 = 64
constexpr int QH = 127;        // hidden activations: 1.0 = 127
constexpr int SHIFT = 6;       // log2(QB)
constexpr std::size_t HEADER_BYTES = 44;

static_assert(MAX_FT % FT_TILE == 0 && FT_TILE % 32 == 0);
static_assert(QB == 1 << SHIFT);

// Hidden-layer widths (L1, L2) with compiled kernels.
constexpr bool supported_hidden(int n) { return n == 16 || n == 32 || n == 64; }

// Layer sizes of a net, as its header gives them.
struct Arch {
    int version = 1;
    int ft = 1024, l1 = 32, l2 = 32, buckets = 8;

    int views() const { return version == 2 ? 2 : 1; }           // points of view feeding L1
    int l1_inputs() const { return views() * ft; }               // N above
    int hidden_buckets() const { return version == 2 ? buckets : 1; }
    std::size_t file_bytes() const {
        const std::size_t hb = std::size_t(hidden_buckets());
        return HEADER_BYTES + 2 * std::size_t(ft) * (1 + INPUTS) + hb * (4 * l1 + std::size_t(l1) * l1_inputs()) +
               hb * (4 * l2 + std::size_t(l2) * l1) + std::size_t(buckets) * (4 + 2 * l2);
    }
    std::string describe() const;  // e.g. "v2 2x512 -> 32 -> 32 -> 1, 8 buckets"
};

// Input index of a piece seen from `view`'s side: that side moves up, black's view is rotated 180 degrees.
constexpr int feature(Color view, Color piece, bool king, int sq) {
    return ((piece == view ? 0 : 2) + king) * 32 + (view == BLACK ? 31 - sq : sq);
}

// Bucket for the number of pieces on the board (1..24): (pieces - 1) / 3 with 8 buckets.
constexpr int bucket(int pieces, int buckets) {
    return std::min(std::max(pieces - 1, 0) * buckets / 24, buckets - 1);
}

// Feature-transformer outputs for both colours' points of view, indexed by colour (the net's first FT entries).
struct alignas(64) Accumulator {
    std::int16_t v[2][MAX_FT];
};

class Network {
public:
    // nullptr if the data is not a valid net for this build; `error` (optional) says why.
    static std::shared_ptr<const Network> load(const unsigned char* data, std::size_t size,
                                               std::string* error = nullptr);

    const Arch& arch() const { return arch_; }
    // Whether evaluate() reads the opponent's point of view too (version 2).
    bool both_views() const { return arch_.views() == 2; }

    void refresh(Accumulator& acc, const Position& pos) const;
    // One point of view only.
    void refresh(std::int16_t* v, const Position& pos, Color view) const;
    // Score from the side to move's point of view, in centi-men. Reads acc.v[stm], and in version 2 acc.v[~stm].
    Score evaluate(const Accumulator& acc, Color stm, int pieces) const { return (this->*forward_)(acc, stm, pieces); }
    // Full refresh, then evaluate.
    Score evaluate(const Position& pos) const;

    // One move's change to one point of view: the feature added and those removed.
    struct Delta {
        int add, nsub;
        int sub[13];
    };
    // Applies n moves to one point of view in one pass: dst[0] = src + delta[0], dst[i] = dst[i - 1] + delta[i].
    void update(const std::int16_t* src, std::int16_t* const* dst, const Delta* delta, int n) const;

    int out_scale() const { return out_scale_; }

    // Times each stage of the forward pass and the accumulator updates over `positions`, and prints them with
    // the sparsity of the activations (regicide nnuebench).
    void profile(const std::vector<Position>& positions, std::ostream& out) const;

private:
    struct AlignedFree {
        void operator()(void* p) const { ::operator delete(p, std::align_val_t(64)); }
    };
    template <class T>
    using Array = std::unique_ptr<T[], AlignedFree>;
    template <class T>
    static Array<T> allocate(std::size_t n) {
        return Array<T>(static_cast<T*>(::operator new(n * sizeof(T), std::align_val_t(64))));
    }

    // Activations of one forward pass.
    struct alignas(64) Forward {
        std::int8_t a[2 * MAX_FT];          // L1's input: feature-transformer outputs
        std::uint16_t nnz[2 * MAX_FT / 4];  // SIMD builds: the 4-byte chunks of `a` that are not all zero
        int count = 0;                      // ... and how many there are
        std::int8_t h1[MAX_HIDDEN];
        std::int8_t h2[MAX_HIDDEN];
    };
    // SCReLU of one point of view into a[0, FT); SIMD builds append its non-zero chunks, numbered from `base`, to
    // nnz[count..]. Returns the new count.
    int transform(const std::int16_t* v, std::int8_t* a, std::uint16_t* nnz, int count, int base) const;
    void transform(const Accumulator& acc, Color stm, Forward& f) const;
    // The layers after the feature transformer, for hidden bucket hb / bucket b, compiled per L1 and L2 width.
    template <int L1>
    void hidden1(Forward& f, int hb) const;
    template <int L1, int L2>
    void hidden2(Forward& f, int hb) const;
    template <int L2>
    Score output(const Forward& f, int b) const;
    template <int L1, int L2>
    Score forward(const Accumulator& acc, Color stm, int pieces) const;

    const std::int16_t* ft_row(int feature) const { return ft_weight_.get() + std::size_t(feature) * arch_.ft; }

    Arch arch_;
    int out_scale_ = 1;
    Score (Network::*forward_)(const Accumulator&, Color, int) const = nullptr;  // forward<L1, L2>
    Array<std::int16_t> ft_bias_;    // [FT]
    Array<std::int16_t> ft_weight_;  // [INPUTS][FT]
    // Hidden-layer weights per hidden bucket, each in the order the affine kernels read them (see weight_index in
    // nnue.cpp).
    Array<std::int8_t> l1_weight_;   // [HB][L1 * N]
    Array<std::int8_t> l2_weight_;   // [HB][L2 * L1]
    Array<std::int32_t> l1_bias_;    // [HB][L1]
    Array<std::int32_t> l2_bias_;    // [HB][L2]
    Array<std::int16_t> out_weight_;  // [BUCKETS][L2]
    Array<std::int32_t> out_bias_;    // [BUCKETS]
};

// Accumulators along the current search path, one entry per ply. The search records each move with push();
// accumulators are computed lazily when a position is evaluated, from the nearest computed ancestor, and one point
// of view at a time: with a version 1 net a leaf only needs its side to move's, the other is computed once a child
// needs it.
class AccumulatorStack {
public:
    static constexpr int SIZE = MAX_PLY + 8;

    // Refreshes ply 0 from the root position.
    void reset(const Network& net, const Position& root);
    // Records the move that leads from `parent` (at ply - 1) to the position at `ply`.
    void push(int ply, const Position& parent, const Move& m) {
        Entry& e = entries_[ply];
        e.computed[BLACK] = e.computed[WHITE] = false;
        e.us = parent.side_to_move();
        e.from = m.from;
        e.to = m.to;
        e.king = (parent.kings() >> m.from) & 1;
        e.captures = m.captures;
        e.captured_kings = m.captures & parent.kings();
    }
    // Brings the points of view the net reads of the accumulator at `ply` (holding `pos`) up to date.
    const Accumulator& get(const Network& net, const Position& pos, int ply) {
        Entry& e = entries_[ply];
        const Color stm = pos.side_to_move();
        if (!e.computed[stm]) compute(net, pos, ply, stm);
        if (net.both_views() && !e.computed[~stm]) compute(net, pos, ply, ~stm);
        return e.acc;
    }
    Score evaluate(const Network& net, const Position& pos, int ply) {
        return net.evaluate(get(net, pos, ply), pos.side_to_move(), pos.piece_count());
    }

private:
    struct Entry {
        Accumulator acc;
        Bitboard captures, captured_kings;
        std::uint8_t from, to;
        Color us;
        bool king;  // the moved piece was a king before the move
        bool computed[2] = {false, false};  // per point of view
    };
    static Network::Delta delta(const Entry& e, Color view);
    // Brings one point of view at `ply` up to date (it is not yet).
    void compute(const Network& net, const Position& pos, int ply, Color view);

    Entry entries_[SIZE];
};

}  // namespace regicide::nnue
