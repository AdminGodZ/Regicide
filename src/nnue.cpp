#include "nnue.hpp"

#include <bit>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <numeric>
#include <ostream>
#include <random>
#include <sstream>

#include "movegen.hpp"

#if defined(__ARM_NEON) && defined(__ARM_FEATURE_DOTPROD)
#include <arm_neon.h>
#define REGICIDE_NNUE_NEON 1
#define REGICIDE_NNUE_SIMD 1
#elif defined(__AVX2__)
#include <immintrin.h>
#define REGICIDE_NNUE_AVX2 1
#define REGICIDE_NNUE_SIMD 1
#endif

namespace regicide::nnue {

namespace {

std::uint32_t read_u32(const unsigned char* p) {
    std::uint32_t v;
    std::memcpy(&v, p, 4);
    return v;
}

#if REGICIDE_NNUE_SIMD
constexpr bool SIMD = true;
#else
constexpr bool SIMD = false;
#endif

// Where weight w[out][in] of an N -> M layer is kept. The SIMD kernels go through the inputs in 4-byte chunks and
// multiply each chunk, broadcast, by the weights of 4 (NEON) or 8 (AVX2) outputs with one dot-product instruction,
// so for each chunk they keep the 4 weights of every output in turn. The portable path keeps the file's order.
constexpr std::size_t weight_index(int out, int in, int N, int M) {
    return SIMD ? std::size_t(in / 4) * M * 4 + out * 4 + in % 4 : std::size_t(out) * N + in;
}

#if REGICIDE_NNUE_SIMD
// For each 8-bit mask, the positions of its set bits and how many there are (find_nnz goes 8 chunks at a time).
struct NnzTable {
    alignas(16) std::uint16_t index[256][8];
    std::uint8_t count[256];
};

constexpr NnzTable make_nnz_table() {
    NnzTable t{};
    for (int m = 0; m < 256; ++m)
        for (int b = 0, k = 0; b < 8; ++b)
            if ((m >> b) & 1) {
                t.index[m][k++] = std::uint16_t(b);
                t.count[m] = std::uint8_t(k);
            }
    return t;
}

constexpr NnzTable NNZ_TABLE = make_nnz_table();
#endif

#if REGICIDE_NNUE_NEON || REGICIDE_NNUE_AVX2
// An order of the feature-transformer outputs in which the 4 outputs of a chunk tend to be zero together, so that
// FT -> L1 has fewer non-zero chunks to multiply. Reordering the outputs only permutes integer sums, so no
// evaluation changes. The order comes from the SCReLU outputs over random playouts: each chunk starts with the
// least active output left and greedily adds the three that keep it all zero in the most positions.
// order[new index] = index in the file.
std::vector<int> sparse_order(const Network& net) {
    constexpr int SAMPLES = 4096, WORDS = SAMPLES / 64;
    std::vector<std::uint64_t> active(std::size_t(FT) * WORDS);  // bit s of output i: non-zero in sample s
    auto row = [&](int i) { return active.data() + std::size_t(i) * WORDS; };
    std::mt19937_64 rng(1);
    alignas(64) std::int16_t v[FT];
    for (int s = 0; s < SAMPLES; ++s) {
        Position pos;
        for (int plies = int(rng() % 80); plies > 0; --plies) {
            MoveList list;
            generate_moves(pos, list);
            if (list.empty()) break;
            pos.make_move(list[rng() % list.size()]);
        }
        net.refresh(v, pos, pos.side_to_move());
        for (int i = 0; i < FT; ++i) {
            const int c = std::clamp<int>(v[i], 0, QA);
            if ((c * c) >> 9) row(i)[s / 64] |= std::uint64_t(1) << (s % 64);
        }
    }
    auto popcount = [&](const std::uint64_t* a, const std::uint64_t* b) {
        int n = 0;
        for (int w = 0; w < WORDS; ++w) n += std::popcount(a[w] | b[w]);
        return n;
    };
    std::vector<int> by_activity(FT), order;
    std::iota(by_activity.begin(), by_activity.end(), 0);
    std::vector<int> activity(FT);
    for (int i = 0; i < FT; ++i) activity[i] = popcount(row(i), row(i));
    std::stable_sort(by_activity.begin(), by_activity.end(), [&](int a, int b) { return activity[a] < activity[b]; });
    std::vector<bool> used(FT);
    std::uint64_t chunk[WORDS];
    for (int next = 0; int(order.size()) < FT;) {
        while (used[by_activity[next]]) ++next;
        int pick = by_activity[next];
        std::fill(chunk, chunk + WORDS, 0);
        for (int k = 0; k < 4; ++k) {
            if (k > 0) {
                int best = SAMPLES + 1;
                for (int i = 0; i < FT; ++i)
                    if (!used[i])
                        if (const int n = popcount(chunk, row(i)); n < best) best = n, pick = i;
            }
            used[pick] = true;
            order.push_back(pick);
            for (int w = 0; w < WORDS; ++w) chunk[w] |= row(pick)[w];
        }
    }
    return order;
}
#endif

#if REGICIDE_NNUE_NEON
// Lists the 4-byte chunks of a[N] that are not all zero, in order, and returns how many there are.
template <int N>
inline int find_nnz(const std::int8_t* a, std::uint16_t* nnz) {
    static_assert(N % 32 == 0);
    const uint32x4_t lo_bits = {1, 2, 4, 8}, hi_bits = {16, 32, 64, 128};
    uint16x8_t base = vdupq_n_u16(0);
    int count = 0;
    for (int i = 0; i < N; i += 32) {
        const uint32x4_t c0 = vreinterpretq_u32_s8(vld1q_s8(a + i)), c1 = vreinterpretq_u32_s8(vld1q_s8(a + i + 16));
        const unsigned mask =
            vaddvq_u32(vorrq_u32(vandq_u32(vtstq_u32(c0, c0), lo_bits), vandq_u32(vtstq_u32(c1, c1), hi_bits)));
        vst1q_u16(nnz + count, vaddq_u16(base, vld1q_u16(NNZ_TABLE.index[mask])));
        count += NNZ_TABLE.count[mask];
        base = vaddq_u16(base, vdupq_n_u16(8));
    }
    return count;
}

// out[j] = bias[j] + the sum over the chunks c = chunk(k), k < count, of a[4c..4c+3] . w[j][4c..4c+3], for j < M
// (int8 inputs and weights, int32 sums). Narrow layers keep two sets of sums, so consecutive chunks do not wait
// on each other.
template <int M, class Chunk>
inline void affine_chunks(const std::int8_t* a, Chunk chunk, int count, const std::int8_t* w,
                          const std::int32_t* bias, std::int32_t* out) {
    static_assert(M % 4 == 0);
    constexpr int R = M / 4, SETS = M <= 32 ? 2 : 1;
    int32x4_t s[SETS][R];
    for (int r = 0; r < R; ++r) {
        s[0][r] = vld1q_s32(bias + 4 * r);
        if constexpr (SETS == 2) s[1][r] = vdupq_n_s32(0);
    }
    auto step = [&](int set, int c) {
        std::int32_t x4;
        std::memcpy(&x4, a + 4 * c, 4);
        const int8x16_t x = vreinterpretq_s8_s32(vdupq_n_s32(x4));
        const std::int8_t* wc = w + c * M * 4;
        for (int r = 0; r < R; ++r) s[set][r] = vdotq_s32(s[set][r], vld1q_s8(wc + 16 * r), x);
    };
    int k = 0;
    for (; k + SETS <= count; k += SETS)
        for (int set = 0; set < SETS; ++set) step(set, chunk(k + set));
    if (k < count) step(0, chunk(k));
    if constexpr (SETS == 2)
        for (int r = 0; r < R; ++r) s[0][r] = vaddq_s32(s[0][r], s[1][r]);
    for (int r = 0; r < R; ++r) vst1q_s32(out + 4 * r, s[0][r]);
}
#elif REGICIDE_NNUE_AVX2
// As the NEON version, 8 chunks per 32-byte load.
template <int N>
inline int find_nnz(const std::int8_t* a, std::uint16_t* nnz) {
    static_assert(N % 32 == 0);
    const __m256i zero = _mm256_setzero_si256();
    __m128i base = _mm_setzero_si128();
    int count = 0;
    for (int i = 0; i < N; i += 32) {
        // Activations are 0..127, so a chunk read as int32 is non-zero exactly when it is positive.
        const __m256i c = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(a + i));
        const unsigned mask = unsigned(_mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpgt_epi32(c, zero))));
        const __m128i index = _mm_load_si128(reinterpret_cast<const __m128i*>(NNZ_TABLE.index[mask]));
        _mm_storeu_si128(reinterpret_cast<__m128i*>(nnz + count), _mm_add_epi16(base, index));
        count += NNZ_TABLE.count[mask];
        base = _mm_add_epi16(base, _mm_set1_epi16(8));
    }
    return count;
}

// acc plus, in each 32-bit lane, the dot product of 4 unsigned bytes of x with 4 signed bytes of w.
inline __m256i dot4(__m256i acc, __m256i x, __m256i w) {
#if defined(__AVXVNNI__)
    return _mm256_dpbusd_avx_epi32(acc, x, w);
#elif defined(__AVX512VNNI__) && defined(__AVX512VL__)
    return _mm256_dpbusd_epi32(acc, x, w);
#else
    // maddubs saturates its 16-bit pair sums, which stay exact here: inputs are 0..127 and weights -128..127, so
    // |x0 w0 + x1 w1| <= 2 * 127 * 128 < 2^15.
    return _mm256_add_epi32(acc, _mm256_madd_epi16(_mm256_maddubs_epi16(x, w), _mm256_set1_epi16(1)));
#endif
}

// As the NEON version, 8 outputs per instruction.
template <int M, class Chunk>
inline void affine_chunks(const std::int8_t* a, Chunk chunk, int count, const std::int8_t* w,
                          const std::int32_t* bias, std::int32_t* out) {
    static_assert(M % 8 == 0);
    constexpr int R = M / 8, SETS = M <= 32 ? 2 : 1;
    __m256i s[SETS][R];
    for (int r = 0; r < R; ++r) {
        s[0][r] = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(bias + 8 * r));
        if constexpr (SETS == 2) s[1][r] = _mm256_setzero_si256();
    }
    auto step = [&](int set, int c) {
        std::int32_t x4;
        std::memcpy(&x4, a + 4 * c, 4);
        const __m256i x = _mm256_set1_epi32(x4);
        const std::int8_t* wc = w + c * M * 4;
        for (int r = 0; r < R; ++r)
            s[set][r] = dot4(s[set][r], x, _mm256_loadu_si256(reinterpret_cast<const __m256i*>(wc + 32 * r)));
    };
    int k = 0;
    for (; k + SETS <= count; k += SETS)
        for (int set = 0; set < SETS; ++set) step(set, chunk(k + set));
    if (k < count) step(0, chunk(k));
    if constexpr (SETS == 2)
        for (int r = 0; r < R; ++r) s[0][r] = _mm256_add_epi32(s[0][r], s[1][r]);
    for (int r = 0; r < R; ++r) _mm256_storeu_si256(reinterpret_cast<__m256i*>(out + 8 * r), s[0][r]);
}
#else
// out[j] = sum of a[i] * w[j][i] over i < N, for j < M (int8 inputs and weights, int32 sums). Four rows at a
// time, so the vectorised loop keeps independent accumulators instead of one dependency chain per row.
template <int N, int M>
inline void affine(const std::int8_t* a, const std::int8_t* w, std::int32_t* out) {
    for (int j = 0; j < M; j += 4) {
        const std::int8_t *w0 = w + j * N, *w1 = w0 + N, *w2 = w1 + N, *w3 = w2 + N;
        std::int32_t s0 = 0, s1 = 0, s2 = 0, s3 = 0;
        for (int i = 0; i < N; ++i) {
            const std::int32_t x = a[i];
            s0 += x * w0[i];
            s1 += x * w1[i];
            s2 += x * w2[i];
            s3 += x * w3[i];
        }
        out[j] = s0;
        out[j + 1] = s1;
        out[j + 2] = s2;
        out[j + 3] = s3;
    }
}
#endif

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
    // SIMD builds reorder the feature-transformer outputs (see sparse_order).
    std::vector<int> order(FT);
    std::iota(order.begin(), order.end(), 0);
#if REGICIDE_NNUE_SIMD
    order = sparse_order(*net);
    const auto file = std::make_unique<Network>(*net);
    for (int i = 0; i < FT; ++i) {
        net->ft_bias_[i] = file->ft_bias_[order[i]];
        for (int f = 0; f < INPUTS; ++f) net->ft_weight_[f][i] = file->ft_weight_[f][order[i]];
    }
#endif
    // Hidden-layer weights come as one row per output, and are kept in the kernels' order.
    auto take_weights = [&](std::int8_t* dst, int N, int M, const int* in_order) {
        for (int j = 0; j < M; ++j)
            for (int i = 0; i < N; ++i) dst[weight_index(j, i, N, M)] = std::int8_t(p[j * N + in_order[i]]);
        p += N * M;
    };
    take(net->l1_bias_, sizeof net->l1_bias_);
    take_weights(net->l1_weight_, FT, L1, order.data());
    take(net->l2_bias_, sizeof net->l2_bias_);
    std::vector<int> identity(L1);
    std::iota(identity.begin(), identity.end(), 0);
    take_weights(net->l2_weight_, L1, L2, identity.data());
    take(net->out_bias_, sizeof net->out_bias_);
    take(net->out_weight_, sizeof net->out_weight_);
    return net;
}

void Network::refresh(Accumulator& acc, const Position& pos) const {
    for (Color view : {BLACK, WHITE}) refresh(acc.v[view], pos, view);
}

void Network::refresh(std::int16_t* v, const Position& pos, Color view) const {
    std::memcpy(v, ft_bias_, sizeof ft_bias_);
    for (Color c : {BLACK, WHITE})
        for (Bitboard b = pos.pieces(c); b;) {
            const int sq = pop_lsb(b);
            const std::int16_t* w = ft_weight_[feature(view, c, (pos.kings() >> sq) & 1, sq)];
            for (int i = 0; i < FT; ++i) v[i] = std::int16_t(v[i] + w[i]);
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

// SCReLU of one point of view's accumulator into f.a; SIMD builds also list the non-zero 4-byte chunks of f.a.
void Network::transform(const std::int16_t* v, Forward& f) const {
    for (int i = 0; i < FT; ++i) {
        const int c = std::clamp<int>(v[i], 0, QA);
        f.a[i] = std::int8_t((c * c) >> 9);
    }
#if REGICIDE_NNUE_SIMD
    f.count = find_nnz<FT>(f.a, f.nnz);
#endif
}

// FT -> L1: SIMD builds only visit the non-zero chunks of f.a.
void Network::hidden1(Forward& f) const {
    std::int32_t s[L1];
#if REGICIDE_NNUE_SIMD
    affine_chunks<L1>(f.a, [&](int k) { return f.nnz[k]; }, f.count, l1_weight_, l1_bias_, s);
#else
    affine<FT, L1>(f.a, l1_weight_, s);
    for (int j = 0; j < L1; ++j) s[j] += l1_bias_[j];
#endif
    for (int j = 0; j < L1; ++j) f.h1[j] = std::int8_t(std::clamp(s[j] >> SHIFT, 0, QH));
}

void Network::hidden2(Forward& f) const {
    std::int32_t s[L2];
#if REGICIDE_NNUE_SIMD
    affine_chunks<L2>(f.h1, [](int k) { return k; }, L1 / 4, l2_weight_, l2_bias_, s);
#else
    affine<L1, L2>(f.h1, l2_weight_, s);
    for (int j = 0; j < L2; ++j) s[j] += l2_bias_[j];
#endif
    for (int j = 0; j < L2; ++j) f.h2[j] = std::int8_t(std::clamp(s[j] >> SHIFT, 0, QH));
}

Score Network::output(const Forward& f, int pieces) const {
    const int b = bucket(pieces);
    std::int32_t out = out_bias_[b];
    for (int j = 0; j < L2; ++j) out += std::int32_t(f.h2[j]) * out_weight_[b][j];
    const std::int64_t s = std::int64_t(out) * out_scale_ / (QH * QB);
    return Score(std::clamp<std::int64_t>(s, -SCORE_KNOWN_WIN + 1, SCORE_KNOWN_WIN - 1));
}

Score Network::evaluate(const Accumulator& acc, Color stm, int pieces) const {
    Forward f;
    transform(acc.v[stm], f);
    hidden1(f);
    hidden2(f);
    return output(f, pieces);
}

Score Network::evaluate(const Position& pos) const {
    Accumulator acc;
    refresh(acc, pos);
    return evaluate(acc, pos.side_to_move(), pos.piece_count());
}

void AccumulatorStack::reset(const Network& net, const Position& root) {
    net.refresh(entries_[0].acc, root);
    entries_[0].computed[BLACK] = entries_[0].computed[WHITE] = true;
}

void AccumulatorStack::apply(const Network& net, const Entry& parent, Entry& e, Color view) const {
    const Color us = e.us, them = ~us;
    const bool to_king = e.king || (bit(e.to) & promo_row(us));
    int add[1] = {feature(view, us, to_king, e.to)};
    int sub[13];
    int nsub = 0;
    sub[nsub++] = feature(view, us, e.king, e.from);
    for (Bitboard b = e.captures; b;) {
        const int sq = pop_lsb(b);
        sub[nsub++] = feature(view, them, (e.captured_kings >> sq) & 1, sq);
    }
    net.update(parent.acc.v[view], e.acc.v[view], add, 1, sub, nsub);
    e.computed[view] = true;
}

const Accumulator& AccumulatorStack::get(const Network& net, const Position& pos, int ply) {
    const Color view = pos.side_to_move();
    if (!entries_[ply].computed[view]) {
        int k = ply;
        while (k > 0 && !entries_[k].computed[view]) --k;
        if (entries_[k].computed[view]) {
            for (int i = k + 1; i <= ply; ++i) apply(net, entries_[i - 1], entries_[i], view);
        } else {
            net.refresh(entries_[ply].acc.v[view], pos, view);
            entries_[ply].computed[view] = true;
        }
    }
    return entries_[ply].acc;
}

void Network::profile(const std::vector<Position>& positions, std::ostream& out) const {
    const int n = int(positions.size());
    if (n == 0) return;
    // Inputs for every stage, prepared up front: accumulators, forward-pass activations and one random legal move
    // per position as feature lists for both points of view (as AccumulatorStack::apply builds them).
    std::vector<Accumulator> accs(n), next(n);
    std::vector<Forward> fw(n);
    struct Move1 {
        int add[2], sub[2][13], nsub = 0;
    };
    std::vector<Move1> moves(n);
    std::mt19937_64 rng(1);
    for (int i = 0; i < n; ++i) {
        const Position& pos = positions[i];
        refresh(accs[i], pos);
        transform(accs[i].v[pos.side_to_move()], fw[i]);
        hidden1(fw[i]);
        hidden2(fw[i]);
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) continue;
        const Move m = list[rng() % list.size()];
        const Color us = pos.side_to_move();
        const bool king = (pos.kings() >> m.from) & 1, to_king = king || (bit(m.to) & promo_row(us));
        Move1& mv = moves[i];
        for (Color view : {BLACK, WHITE}) {
            int k = 0;
            mv.add[view] = feature(view, us, to_king, m.to);
            mv.sub[view][k++] = feature(view, us, king, m.from);
            for (Bitboard b = m.captures; b;) {
                const int sq = pop_lsb(b);
                mv.sub[view][k++] = feature(view, ~us, (pos.kings() >> sq) & 1, sq);
            }
            mv.nsub = k;
        }
    }

    // Nanoseconds per call of fn(i), the best of 5 rounds of at least 40 ms over all positions.
    std::int64_t sink = 0;
    auto time = [&](auto&& fn) {
        using clock = std::chrono::steady_clock;
        double best = 1e300;
        for (int round = 0; round < 5; ++round) {
            const auto t0 = clock::now();
            long calls = 0;
            do {
                for (int i = 0; i < n; ++i) fn(i);
                calls += n;
            } while (clock::now() - t0 < std::chrono::milliseconds(40));
            best = std::min(best, std::chrono::duration<double, std::nano>(clock::now() - t0).count() / double(calls));
        }
        return best;
    };
    const double t_refresh = time([&](int i) { refresh(next[i], positions[i]); });
    const double t_update = time([&](int i) {
        const Move1& mv = moves[i];
        for (Color view : {BLACK, WHITE})
            update(accs[i].v[view], next[i].v[view], &mv.add[view], 1, mv.sub[view], mv.nsub);
    });
    const double t_eval = time([&](int i) {
        sink += evaluate(accs[i], positions[i].side_to_move(), positions[i].piece_count());
    });
    const double t_transform = time([&](int i) { transform(accs[i].v[positions[i].side_to_move()], fw[i]); });
    const double t_hidden1 = time([&](int i) { hidden1(fw[i]); });
    const double t_hidden2 = time([&](int i) { hidden2(fw[i]); });
    const double t_output = time([&](int i) { sink += output(fw[i], positions[i].piece_count()); });

    // Share of non-zero bytes and non-zero 4-byte chunks in an activation vector.
    auto density = [&](auto member, int size) {
        long bytes = 0, chunks = 0;
        for (const Forward& f : fw) {
            const std::int8_t* x = f.*member;
            for (int i = 0; i < size; i += 4) {
                const int nz = (x[i] != 0) + (x[i + 1] != 0) + (x[i + 2] != 0) + (x[i + 3] != 0);
                bytes += nz;
                chunks += nz > 0;
            }
        }
        std::ostringstream s;
        s << std::fixed << std::setprecision(1) << 100.0 * double(bytes) / (double(n) * size) << "% non-zero bytes, "
          << 100.0 * double(chunks) / (double(n) * size / 4) << "% non-zero chunks ("
          << double(chunks) / n << " of " << size / 4 << ")";
        return s.str();
    };
    out << std::fixed << std::setprecision(1) << "nnue profile: FT " << FT << ", L1 " << L1 << ", L2 " << L2 << ", "
        << n << " positions, ns per call\n"
        << "  refresh (both views)          " << std::setw(7) << t_refresh << '\n'
        << "  update (one move, both views) " << std::setw(7) << t_update << '\n'
        << "  evaluate                      " << std::setw(7) << t_eval << '\n'
        << "    transform (SCReLU)          " << std::setw(7) << t_transform << '\n'
        << "    FT -> L1                    " << std::setw(7) << t_hidden1 << '\n'
        << "    L1 -> L2                    " << std::setw(7) << t_hidden2 << '\n'
        << "    output                      " << std::setw(7) << t_output << '\n'
        << "  FT output: " << density(&Forward::a, FT) << '\n'
        << "  L1 output: " << density(&Forward::h1, L1) << '\n'
        << "  (checksum " << sink % 1000 << ")" << std::endl;
}

}  // namespace regicide::nnue
