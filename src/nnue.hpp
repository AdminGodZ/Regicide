#pragma once

// NNUE evaluation: a small quantised network over the pieces seen from the side to move, oriented like the linear
// eval (the side to move goes "up"; black positions are rotated 180 degrees with colours swapped).
//
//   inputs   128 = [our man, our king, their man, their king] x 32 squares
//   FT       128 -> 256, int16 weights, SCReLU: a = clamp(acc, 0, QA)^2 >> 9 (0..127, 1.0 = 127)
//   L1       256 -> 32,  int8 weights, int32 sums, clipped ReLU: h = clamp(sum >> 6, 0, 127)
//   L2       32 -> 32,   same
//   output   32 -> 1 per bucket (by piece count), int16 weights; score = out * out_scale / (127 * 64)
//
// The search keeps one accumulator per colour's point of view on its stack and updates both from each move;
// the forward pass uses the side to move's. tools/nnue/ trains and exports nets (see its README).
//
// File format (little endian): "RGNN", then u32 version, inputs, ft, l1, l2, buckets, activation, qa, qb,
// out_scale, then ft_bias i16[FT], ft_weight i16[INPUTS][FT], l1_bias i32[L1], l1_weight i8[L1][FT],
// l2_bias i32[L2], l2_weight i8[L2][L1], out_bias i32[BUCKETS], out_weight i16[BUCKETS][L2]. Everything but
// out_scale must match the constants below, so a net for another architecture is rejected.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "eval.hpp"

namespace regicide::nnue {

constexpr int INPUTS = 128;
constexpr int FT = 512;
constexpr int L1 = 32;
constexpr int L2 = 32;
constexpr int BUCKETS = 8;

constexpr int ACTIVATION = 1;  // 1 = SCReLU on the feature transformer (recorded in the header)
constexpr int QA = 255;        // feature transformer: 1.0 = 255
constexpr int QB = 64;         // hidden and output weights: 1.0 = 64
constexpr int QH = 127;        // hidden activations: 1.0 = 127
constexpr int SHIFT = 6;       // log2(QB)
constexpr std::uint32_t VERSION = 1;
constexpr std::size_t HEADER_BYTES = 44;
constexpr std::size_t FILE_BYTES = HEADER_BYTES + 2 * FT + 2 * INPUTS * FT + 4 * L1 + L1 * FT + 4 * L2 + L2 * L1 +
                                   4 * BUCKETS + 2 * BUCKETS * L2;

static_assert(FT % 32 == 0 && L1 % 32 == 0 && L2 % 4 == 0, "layer widths must suit the SIMD kernels");
static_assert(QB == 1 << SHIFT);

// Input index of a piece seen from `view`'s side: that side moves up, black's view is rotated 180 degrees.
constexpr int feature(Color view, Color piece, bool king, int sq) {
    return ((piece == view ? 0 : 2) + king) * 32 + (view == BLACK ? 31 - sq : sq);
}

// Output bucket for the number of pieces on the board (1..24).
constexpr int bucket(int pieces) { return std::min(std::max(pieces - 1, 0) * BUCKETS / 24, BUCKETS - 1); }

// Feature-transformer outputs for both colours' points of view, indexed by colour.
struct alignas(64) Accumulator {
    std::int16_t v[2][FT];
};

class Network {
public:
    // nullptr if the data is not a valid net for this build; `error` (optional) says why.
    static std::shared_ptr<const Network> load(const unsigned char* data, std::size_t size,
                                               std::string* error = nullptr);

    void refresh(Accumulator& acc, const Position& pos) const;
    // Score from the side to move's point of view, in centi-men.
    Score evaluate(const Accumulator& acc, Color stm, int pieces) const;
    // Full refresh, then evaluate.
    Score evaluate(const Position& pos) const;

    // dst = src with the given features removed and added, for one point of view.
    void update(const std::int16_t* src, std::int16_t* dst, const int* add, int nadd, const int* sub, int nsub) const;

    int out_scale() const { return out_scale_; }

private:
    alignas(64) std::int16_t ft_bias_[FT];
    alignas(64) std::int16_t ft_weight_[INPUTS][FT];
    alignas(64) std::int8_t l1_weight_[L1][FT];
    alignas(64) std::int8_t l2_weight_[L2][L1];
    alignas(64) std::int32_t l1_bias_[L1];
    alignas(64) std::int32_t l2_bias_[L2];
    alignas(64) std::int16_t out_weight_[BUCKETS][L2];
    std::int32_t out_bias_[BUCKETS];
    int out_scale_ = 1;
};

// Accumulators along the current search path, one entry per ply. The search records each move with push();
// accumulators are computed lazily when a position is evaluated, from the nearest computed ancestor.
class AccumulatorStack {
public:
    static constexpr int SIZE = MAX_PLY + 8;

    // Refreshes ply 0 from the root position.
    void reset(const Network& net, const Position& root);
    // Records the move that leads from `parent` (at ply - 1) to the position at `ply`.
    void push(int ply, const Position& parent, const Move& m) {
        Entry& e = entries_[ply];
        e.computed = false;
        e.us = parent.side_to_move();
        e.from = m.from;
        e.to = m.to;
        e.king = (parent.kings() >> m.from) & 1;
        e.captures = m.captures;
        e.captured_kings = m.captures & parent.kings();
    }
    // Brings the accumulator at `ply` (holding `pos`) up to date.
    const Accumulator& get(const Network& net, const Position& pos, int ply);
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
        bool computed = false;
    };
    void apply(const Network& net, const Entry& parent, Entry& e) const;

    Entry entries_[SIZE];
};

}  // namespace regicide::nnue
