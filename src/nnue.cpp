#include "nnue.hpp"

#include <cstring>

#if defined(__ARM_NEON) && defined(__ARM_FEATURE_DOTPROD)
#include <arm_neon.h>
#define REGICIDE_NNUE_NEON 1
#elif defined(__AVX2__)
#include <immintrin.h>
#define REGICIDE_NNUE_AVX2 1
#endif

namespace regicide::nnue {

namespace {

std::uint32_t read_u32(const unsigned char* p) {
    std::uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

// out[j] = sum of a[i] * w[j][i] over i < N, for j < M (int8 inputs and weights, int32 sums). Four rows at a
// time, so the vectorised loop keeps independent accumulators instead of one dependency chain per row.
template <int N, int M>
inline void affine(const std::int8_t* a, const std::int8_t (*w)[N], std::int32_t* out) {
#if REGICIDE_NNUE_NEON
    static_assert(N % 16 == 0 && M % 4 == 0);
    for (int j = 0; j < M; j += 4) {
        int32x4_t s0 = vdupq_n_s32(0), s1 = s0, s2 = s0, s3 = s0;
        for (int i = 0; i < N; i += 16) {
            const int8x16_t x = vld1q_s8(a + i);
            s0 = vdotq_s32(s0, x, vld1q_s8(w[j] + i));
            s1 = vdotq_s32(s1, x, vld1q_s8(w[j + 1] + i));
            s2 = vdotq_s32(s2, x, vld1q_s8(w[j + 2] + i));
            s3 = vdotq_s32(s3, x, vld1q_s8(w[j + 3] + i));
        }
        vst1q_s32(out + j, vpaddq_s32(vpaddq_s32(s0, s1), vpaddq_s32(s2, s3)));
    }
#elif REGICIDE_NNUE_AVX2
    // maddubs takes unsigned inputs: fine, since activations are 0..127 (and pair sums stay below 2^15).
    static_assert(N % 32 == 0 && M % 4 == 0);
    const __m256i ones = _mm256_set1_epi16(1);
    for (int j = 0; j < M; j += 4) {
        __m256i s[4] = {_mm256_setzero_si256(), _mm256_setzero_si256(), _mm256_setzero_si256(),
                        _mm256_setzero_si256()};
        for (int i = 0; i < N; i += 32) {
            const __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
            for (int k = 0; k < 4; ++k) {
                const __m256i wk = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(w[j + k] + i));
                s[k] = _mm256_add_epi32(s[k], _mm256_madd_epi16(_mm256_maddubs_epi16(x, wk), ones));
            }
        }
        // Horizontal sums of the four rows into out[j..j+3].
        const __m256i p = _mm256_hadd_epi32(_mm256_hadd_epi32(s[0], s[1]), _mm256_hadd_epi32(s[2], s[3]));
        const __m128i r = _mm_add_epi32(_mm256_castsi256_si128(p), _mm256_extracti128_si256(p, 1));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(out + j), r);
    }
#else
    for (int j = 0; j < M; j += 4) {
        std::int32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        for (int i = 0; i < N; ++i) {
            const std::int32_t x = a[i];
            s0 += x * w[j][i];
            s1 += x * w[j + 1][i];
            s2 += x * w[j + 2][i];
            s3 += x * w[j + 3][i];
        }
        out[j] = s0;
        out[j + 1] = s1;
        out[j + 2] = s2;
        out[j + 3] = s3;
    }
#endif
}

}  // namespace

std::shared_ptr<const Network> Network::load(const unsigned char* data, std::size_t size, std::string* error) {
    auto fail = [&](const std::string& why) -> std::shared_ptr<const Network> {
        if (error) *error = why;
        return nullptr;
    };
    if (size < HEADER_BYTES || std::memcmp(data, "RGNN", 4) != 0) return fail("not an RGNN file");
    const std::uint32_t expect[10] = {VERSION, INPUTS, FT, L1, L2, BUCKETS, ACTIVATION, QA, QB, 0};
    const char* names[10] = {"version", "inputs", "ft", "l1", "l2", "buckets", "activation", "qa", "qb", ""};
    for (int i = 0; i < 9; ++i)
        if (const std::uint32_t got = read_u32(data + 4 + 4 * i); got != expect[i])
            return fail(std::string("net ") + names[i] + " " + std::to_string(got) + ", this build expects " +
                        std::to_string(expect[i]));
    if (size != FILE_BYTES)
        return fail("net size " + std::to_string(size) + ", expected " + std::to_string(FILE_BYTES));

    auto net = std::make_shared<Network>();
    net->out_scale_ = int(read_u32(data + 40));
    const unsigned char* p = data + HEADER_BYTES;
    auto take = [&](void* dst, std::size_t bytes) {
        std::memcpy(dst, p, bytes);
        p += bytes;
    };
    take(net->ft_bias_, sizeof net->ft_bias_);
    take(net->ft_weight_, sizeof net->ft_weight_);
    take(net->l1_bias_, sizeof net->l1_bias_);
    take(net->l1_weight_, sizeof net->l1_weight_);
    take(net->l2_bias_, sizeof net->l2_bias_);
    take(net->l2_weight_, sizeof net->l2_weight_);
    take(net->out_bias_, sizeof net->out_bias_);
    take(net->out_weight_, sizeof net->out_weight_);
    return net;
}

void Network::refresh(Accumulator& acc, const Position& pos) const {
    for (Color view : {BLACK, WHITE}) {
        std::int16_t* v = acc.v[view];
        std::memcpy(v, ft_bias_, sizeof ft_bias_);
        for (Color c : {BLACK, WHITE})
            for (Bitboard b = pos.pieces(c); b;) {
                const int sq = pop_lsb(b);
                const std::int16_t* w = ft_weight_[feature(view, c, (pos.kings() >> sq) & 1, sq)];
                for (int i = 0; i < FT; ++i) v[i] = std::int16_t(v[i] + w[i]);
            }
    }
}

void Network::update(const std::int16_t* src, std::int16_t* dst, const int* add, int nadd, const int* sub,
                     int nsub) const {
    // Fuse the common cases into one pass: a quiet move (one add, one remove) and a single capture.
    if (nadd == 1 && nsub >= 1) {
        const std::int16_t *a = ft_weight_[add[0]], *s = ft_weight_[sub[0]];
        if (nsub >= 2) {
            const std::int16_t* s2 = ft_weight_[sub[1]];
            for (int i = 0; i < FT; ++i) dst[i] = std::int16_t(src[i] + a[i] - s[i] - s2[i]);
            sub += 2;
            nsub -= 2;
        } else {
            for (int i = 0; i < FT; ++i) dst[i] = std::int16_t(src[i] + a[i] - s[i]);
            sub += 1;
            nsub -= 1;
        }
        nadd = 0;
    } else if (src != dst) {
        std::memcpy(dst, src, FT * sizeof(std::int16_t));
    }
    for (int k = 0; k < nadd; ++k)
        for (int i = 0; i < FT; ++i) dst[i] = std::int16_t(dst[i] + ft_weight_[add[k]][i]);
    for (int k = 0; k < nsub; ++k)
        for (int i = 0; i < FT; ++i) dst[i] = std::int16_t(dst[i] - ft_weight_[sub[k]][i]);
}

Score Network::evaluate(const Accumulator& acc, Color stm, int pieces) const {
    alignas(64) std::int8_t a[FT];
    alignas(64) std::int8_t h1[L1];
    alignas(64) std::int8_t h2[L2];
    const std::int16_t* v = acc.v[stm];
    for (int i = 0; i < FT; ++i) {
        const int c = std::clamp<int>(v[i], 0, QA);
        a[i] = std::int8_t((c * c) >> 9);
    }
    std::int32_t s1[L1], s2[L2];
    affine<FT, L1>(a, l1_weight_, s1);
    for (int j = 0; j < L1; ++j) h1[j] = std::int8_t(std::clamp((s1[j] + l1_bias_[j]) >> SHIFT, 0, QH));
    affine<L1, L2>(h1, l2_weight_, s2);
    for (int j = 0; j < L2; ++j) h2[j] = std::int8_t(std::clamp((s2[j] + l2_bias_[j]) >> SHIFT, 0, QH));
    const int b = bucket(pieces);
    std::int32_t out = out_bias_[b];
    for (int j = 0; j < L2; ++j) out += std::int32_t(h2[j]) * out_weight_[b][j];
    const std::int64_t s = std::int64_t(out) * out_scale_ / (QH * QB);
    return Score(std::clamp<std::int64_t>(s, -SCORE_KNOWN_WIN + 1, SCORE_KNOWN_WIN - 1));
}

Score Network::evaluate(const Position& pos) const {
    Accumulator acc;
    refresh(acc, pos);
    return evaluate(acc, pos.side_to_move(), pos.piece_count());
}

void AccumulatorStack::reset(const Network& net, const Position& root) {
    net.refresh(entries_[0].acc, root);
    entries_[0].computed = true;
}

void AccumulatorStack::apply(const Network& net, const Entry& parent, Entry& e) const {
    const Color us = e.us, them = ~us;
    const bool to_king = e.king || (bit(e.to) & promo_row(us));
    for (Color view : {BLACK, WHITE}) {
        int add[1] = {feature(view, us, to_king, e.to)};
        int sub[13];
        int nsub = 0;
        sub[nsub++] = feature(view, us, e.king, e.from);
        for (Bitboard b = e.captures; b;) {
            const int sq = pop_lsb(b);
            sub[nsub++] = feature(view, them, (e.captured_kings >> sq) & 1, sq);
        }
        net.update(parent.acc.v[view], e.acc.v[view], add, 1, sub, nsub);
    }
    e.computed = true;
}

const Accumulator& AccumulatorStack::get(const Network& net, const Position& pos, int ply) {
    if (!entries_[ply].computed) {
        int k = ply;
        while (k > 0 && !entries_[k].computed) --k;
        if (entries_[k].computed) {
            for (int i = k + 1; i <= ply; ++i) apply(net, entries_[i - 1], entries_[i]);
        } else {
            net.refresh(entries_[ply].acc, pos);
            entries_[ply].computed = true;
        }
    }
    return entries_[ply].acc;
}

}  // namespace regicide::nnue
