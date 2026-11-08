#include "nnue.hpp"

#include <array>
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
// For each 8-bit mask, the positions of its set bits and how many there are (the non-zero chunk lists are built 8
// chunks at a time).
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

// An order of the feature-transformer outputs in which the 4 outputs of a chunk tend to be zero together, so that
// FT -> L1 has fewer non-zero chunks to multiply. Reordering the outputs only permutes integer sums, so no
// evaluation changes. The order comes from the SCReLU outputs over random playouts: each chunk starts with the
// least active output left and greedily adds the three that keep it all zero in the most positions.
// order[new index] = index in the file.
std::vector<int> sparse_order(const Network& net) {
    constexpr int SAMPLES = 4096, WORDS = SAMPLES / 64;
    const int ft = net.arch().ft;
    std::vector<std::uint64_t> active(std::size_t(ft) * WORDS);  // bit s of output i: non-zero in sample s
    auto row = [&](int i) { return active.data() + std::size_t(i) * WORDS; };
    std::mt19937_64 rng(1);
    std::vector<std::int16_t> v(ft);
    for (int s = 0; s < SAMPLES; ++s) {
        Position pos;
        for (int plies = int(rng() % 80); plies > 0; --plies) {
            MoveList list;
            generate_moves(pos, list);
            if (list.empty()) break;
            pos.make_move(list[rng() % list.size()]);
        }
        // Version 2 feeds L1 both points of view, so half the samples are the opponent's.
        const Color view = net.both_views() && s % 2 ? ~pos.side_to_move() : pos.side_to_move();
        net.refresh(v.data(), pos, view);
        for (int i = 0; i < ft; ++i) {
            const int c = std::clamp<int>(v[i], 0, QA);
            if ((c * c) >> 9) row(i)[s / 64] |= std::uint64_t(1) << (s % 64);
        }
    }
    auto popcount = [&](const std::uint64_t* a, const std::uint64_t* b) {
        int n = 0;
        for (int w = 0; w < WORDS; ++w) n += std::popcount(a[w] | b[w]);
        return n;
    };
    std::vector<int> by_activity(ft), order;
    std::iota(by_activity.begin(), by_activity.end(), 0);
    std::vector<int> activity(ft);
    for (int i = 0; i < ft; ++i) activity[i] = popcount(row(i), row(i));
    std::stable_sort(by_activity.begin(), by_activity.end(), [&](int a, int b) { return activity[a] < activity[b]; });
    std::vector<bool> used(ft);
    std::uint64_t chunk[WORDS];
    for (int next = 0; int(order.size()) < ft;) {
        while (used[by_activity[next]]) ++next;
        int pick = by_activity[next];
        std::fill(chunk, chunk + WORDS, 0);
        for (int k = 0; k < 4; ++k) {
            if (k > 0) {
                int best = SAMPLES + 1;
                for (int i = 0; i < ft; ++i)
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
// Appends the 4-byte chunks of a[n] that are not all zero, numbered from `base`, to nnz[count..], in order, and
// returns the new count.
inline int find_nnz(const std::int8_t* a, int n, std::uint16_t* nnz, int count, int base_chunk) {
    const __m256i zero = _mm256_setzero_si256();
    __m128i base = _mm_set1_epi16(std::int16_t(base_chunk));
    for (int i = 0; i < n; i += 32) {
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
template <int M>
inline void affine(const std::int8_t* a, int N, const std::int8_t* w, std::int32_t* out) {
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

// Calls f(std::integral_constant<int, n>) for a supported hidden-layer width n (supported_hidden).
template <class F>
inline void with_width(int n, F&& f) {
    switch (n) {
    case 16: f(std::integral_constant<int, 16>{}); break;
    case 32: f(std::integral_constant<int, 32>{}); break;
    default: f(std::integral_constant<int, 64>{}); break;
    }
}

}  // namespace

std::string Arch::describe() const {
    std::ostringstream s;
    s << 'v' << version << ' ' << (views() == 2 ? "2x" : "") << ft << " -> " << l1 << " -> " << l2 << " -> 1, "
      << buckets << (version == 2 ? " buckets (all layers)" : " output buckets");
    return s.str();
}

std::shared_ptr<const Network> Network::load(const unsigned char* data, std::size_t size, std::string* error) {
    auto fail = [&](const std::string& why) -> std::shared_ptr<const Network> {
        if (error) *error = why;
        return nullptr;
    };
    if (size < HEADER_BYTES || std::memcmp(data, "RGNN", 4) != 0) return fail("not an RGNN file");
    auto field = [&](int i) { return read_u32(data + 4 + 4 * i); };
    const std::uint32_t version = field(0);
    if (version != 1 && version != 2)
        return fail("net version " + std::to_string(version) + ", this build supports versions 1 and 2");
    const char* names[9] = {"version", "inputs", "ft", "l1", "l2", "buckets", "activation", "qa", "qb"};
    const std::uint32_t expect[9] = {version, INPUTS, 0, 0, 0, 0, ACTIVATION, QA, QB};
    for (int i : {1, 6, 7, 8})
        if (field(i) != expect[i])
            return fail(std::string("net ") + names[i] + " " + std::to_string(field(i)) + ", this build expects " +
                        std::to_string(expect[i]));
    // Width fields are checked before they are used as ints.
    for (int i = 2; i <= 5; ++i)
        if (field(i) == 0 || field(i) > 1u << 16)
            return fail(std::string("net ") + names[i] + " " + std::to_string(field(i)) + " is out of range");
    Arch arch;
    arch.version = int(version);
    arch.ft = int(field(2));
    arch.l1 = int(field(3));
    arch.l2 = int(field(4));
    arch.buckets = int(field(5));
    const std::string what = "net " + arch.describe() + ": ";
    if (arch.ft % FT_TILE != 0 || arch.ft > MAX_FT)
        return fail(what + "ft must be a multiple of " + std::to_string(FT_TILE) + " up to " + std::to_string(MAX_FT) +
                    " (raise the limit with cmake -DREGICIDE_NNUE_MAX_FT)");
    if (!supported_hidden(arch.l1) || !supported_hidden(arch.l2))
        return fail(what + "l1 and l2 must be 16, 32 or 64");
    if (arch.buckets > MAX_BUCKETS) return fail(what + "at most " + std::to_string(MAX_BUCKETS) + " buckets");
    if (size != arch.file_bytes())
        return fail(what + "size " + std::to_string(size) + ", expected " + std::to_string(arch.file_bytes()));

    auto net = std::make_shared<Network>();
    net->arch_ = arch;
    with_width(arch.l1, [&](auto l1) {
        with_width(arch.l2, [&](auto l2) { net->forward_ = &Network::forward<l1(), l2()>; });
    });
    net->out_scale_ = int(read_u32(data + 40));
    const int ft = arch.ft, l1 = arch.l1, l2 = arch.l2, n1 = arch.l1_inputs(), hb = arch.hidden_buckets();
    net->ft_bias_ = allocate<std::int16_t>(ft);
    net->ft_weight_ = allocate<std::int16_t>(std::size_t(INPUTS) * ft);
    net->l1_bias_ = allocate<std::int32_t>(std::size_t(hb) * l1);
    net->l1_weight_ = allocate<std::int8_t>(std::size_t(hb) * l1 * n1);
    net->l2_bias_ = allocate<std::int32_t>(std::size_t(hb) * l2);
    net->l2_weight_ = allocate<std::int8_t>(std::size_t(hb) * l2 * l1);
    net->out_bias_ = allocate<std::int32_t>(arch.buckets);
    net->out_weight_ = allocate<std::int16_t>(std::size_t(arch.buckets) * l2);

    const unsigned char* p = data + HEADER_BYTES;
    auto take = [&](void* dst, std::size_t bytes) {
        std::memcpy(dst, p, bytes);
        p += bytes;
    };
    take(net->ft_bias_.get(), 2 * std::size_t(ft));
    take(net->ft_weight_.get(), 2 * std::size_t(INPUTS) * ft);
    // SIMD builds reorder the feature-transformer outputs (see sparse_order); both points of view the same way.
    std::vector<int> order(ft);
    std::iota(order.begin(), order.end(), 0);
#if REGICIDE_NNUE_SIMD
    order = sparse_order(*net);
    const std::vector<std::int16_t> bias(net->ft_bias_.get(), net->ft_bias_.get() + ft),
        weight(net->ft_weight_.get(), net->ft_weight_.get() + std::size_t(INPUTS) * ft);
    for (int i = 0; i < ft; ++i) {
        net->ft_bias_[i] = bias[order[i]];
        for (std::size_t row = 0; row < std::size_t(INPUTS) * ft; row += ft)
            net->ft_weight_[row + i] = weight[row + order[i]];
    }
#endif
    // Hidden-layer weights come as one row per output, bucket after bucket, and are kept in the kernels' order.
    auto take_weights = [&](std::int8_t* dst, int N, int M, const int* in_order) {
        for (int b = 0; b < hb; ++b, dst += std::size_t(N) * M, p += std::size_t(N) * M)
            for (int j = 0; j < M; ++j)
                for (int i = 0; i < N; ++i)
                    dst[weight_index(j, i, N, M)] = std::int8_t(p[std::size_t(j) * N + in_order[i]]);
    };
    std::vector<int> l1_order(n1);
    for (int i = 0; i < n1; ++i) l1_order[i] = i / ft * ft + order[i % ft];
    take(net->l1_bias_.get(), 4 * std::size_t(hb) * l1);
    take_weights(net->l1_weight_.get(), n1, l1, l1_order.data());
    take(net->l2_bias_.get(), 4 * std::size_t(hb) * l2);
    std::vector<int> identity(l1);
    std::iota(identity.begin(), identity.end(), 0);
    take_weights(net->l2_weight_.get(), l1, l2, identity.data());
    take(net->out_bias_.get(), 4 * std::size_t(arch.buckets));
    take(net->out_weight_.get(), 2 * std::size_t(arch.buckets) * l2);
    return net;
}

void Network::refresh(Accumulator& acc, const Position& pos) const {
    for (Color view : {BLACK, WHITE}) refresh(acc.v[view], pos, view);
}

void Network::refresh(std::int16_t* v, const Position& pos, Color view) const {
    const int ft = arch_.ft;
    std::memcpy(v, ft_bias_.get(), 2 * std::size_t(ft));
    for (Color c : {BLACK, WHITE})
        for (Bitboard b = pos.pieces(c); b;) {
            const int sq = pop_lsb(b);
            const std::int16_t* w = ft_row(feature(view, c, (pos.kings() >> sq) & 1, sq));
            for (int i = 0; i < ft; ++i) v[i] = std::int16_t(v[i] + w[i]);
        }
}

void Network::update(const std::int16_t* src, std::int16_t* const* dst, const Delta* delta, int n) const {
    // Tile by tile, so a tile stays in registers across the moves.
    // Locals, as the stores to dst could alias the members.
    constexpr int TILE = FT_TILE;
    const int ft = arch_.ft;
    const std::int16_t* weight = ft_weight_.get();
    auto row = [&](int feature) { return weight + std::size_t(feature) * ft; };
    for (int t = 0; t < ft; t += TILE) {
        std::int16_t v[TILE];
        std::memcpy(v, src + t, sizeof v);
        for (int m = 0; m < n; ++m) {
            const Delta& d = delta[m];
            const std::int16_t *a = row(d.add) + t, *s = row(d.sub[0]) + t;
            for (int i = 0; i < TILE; ++i) v[i] = std::int16_t(v[i] + a[i] - s[i]);
            for (int k = 1; k < d.nsub; ++k) {
                s = row(d.sub[k]) + t;
                for (int i = 0; i < TILE; ++i) v[i] = std::int16_t(v[i] - s[i]);
            }
            std::memcpy(dst[m] + t, v, sizeof v);
        }
    }
}

int Network::transform(const std::int16_t* v, std::int8_t* out, std::uint16_t* nnz, int count, int base) const {
    const int ft = arch_.ft;
#if REGICIDE_NNUE_NEON
    static_assert(QA == 255);
    std::uint8_t* a = reinterpret_cast<std::uint8_t*>(out);
    uint16x8_t index_base = vdupq_n_u16(std::uint16_t(base));
    for (int i = 0; i < ft; i += 32) {
        uint8x16_t o[2];
        for (int h = 0; h < 2; ++h) {
            // clamp(x, 0, 255) by narrowing with unsigned saturation, then c^2 >> 9 = (c^2 >> 8) >> 1.
            const int16_t* x = v + i + 16 * h;
            const uint8x16_t c = vqmovun_high_s16(vqmovun_s16(vld1q_s16(x)), vld1q_s16(x + 8));
            const uint16x8_t lo = vmull_u8(vget_low_u8(c), vget_low_u8(c)), hi = vmull_high_u8(c, c);
            o[h] = vshrq_n_u8(vshrn_high_n_u16(vshrn_n_u16(lo, 8), hi, 8), 1);
            vst1q_u8(a + i + 16 * h, o[h]);
        }
        // One byte per chunk that saturating narrowing keeps non-zero, as 0 or 1, then gathered into 8 bits.
        const uint16x8_t n16 = vqmovn_high_u32(vqmovn_u32(vreinterpretq_u32_u8(o[0])), vreinterpretq_u32_u8(o[1]));
        const uint8x8_t nz = vmin_u8(vqmovn_u16(n16), vdup_n_u8(1));
        const unsigned mask = unsigned((vget_lane_u64(vreinterpret_u64_u8(nz), 0) * 0x0102040810204080ull) >> 56);
        vst1q_u16(nnz + count, vaddq_u16(index_base, vld1q_u16(NNZ_TABLE.index[mask])));
        count += NNZ_TABLE.count[mask];
        index_base = vaddq_u16(index_base, vdupq_n_u16(8));
    }
#else
    for (int i = 0; i < ft; ++i) {
        const int c = std::clamp<int>(v[i], 0, QA);
        out[i] = std::int8_t((c * c) >> 9);
    }
#if REGICIDE_NNUE_AVX2
    count = find_nnz(out, ft, nnz, count, base);
#endif
#endif
    (void)nnz;
    (void)base;
    return count;
}

// L1's input: SCReLU of the side to move's point of view, then (version 2) of the opponent's.
void Network::transform(const Accumulator& acc, Color stm, Forward& f) const {
    f.count = transform(acc.v[stm], f.a, f.nnz, 0, 0);
    if (both_views()) f.count = transform(acc.v[~stm], f.a + arch_.ft, f.nnz, f.count, arch_.ft / 4);
}

// L1 of hidden bucket hb: SIMD builds only visit the non-zero chunks of f.a.
template <int L1>
void Network::hidden1(Forward& f, int hb) const {
    const int n = arch_.l1_inputs();
    const std::int8_t* w = l1_weight_.get() + std::size_t(hb) * L1 * n;
    const std::int32_t* bias = l1_bias_.get() + std::size_t(hb) * L1;
    std::int32_t s[L1];
#if REGICIDE_NNUE_SIMD
    affine_chunks<L1>(f.a, [&](int k) { return f.nnz[k]; }, f.count, w, bias, s);
#else
    affine<L1>(f.a, n, w, s);
    for (int j = 0; j < L1; ++j) s[j] += bias[j];
#endif
    for (int j = 0; j < L1; ++j) f.h1[j] = std::int8_t(std::clamp(s[j] >> SHIFT, 0, QH));
}

template <int L1, int L2>
void Network::hidden2(Forward& f, int hb) const {
    const std::int8_t* w = l2_weight_.get() + std::size_t(hb) * L2 * L1;
    const std::int32_t* bias = l2_bias_.get() + std::size_t(hb) * L2;
    std::int32_t s[L2];
#if REGICIDE_NNUE_SIMD
    affine_chunks<L2>(f.h1, [](int k) { return k; }, L1 / 4, w, bias, s);
#else
    affine<L2>(f.h1, L1, w, s);
    for (int j = 0; j < L2; ++j) s[j] += bias[j];
#endif
    for (int j = 0; j < L2; ++j) f.h2[j] = std::int8_t(std::clamp(s[j] >> SHIFT, 0, QH));
}

template <int L2>
Score Network::output(const Forward& f, int b) const {
    const std::int16_t* w = out_weight_.get() + std::size_t(b) * L2;
    std::int32_t out = out_bias_[b];
    for (int j = 0; j < L2; ++j) out += std::int32_t(f.h2[j]) * w[j];
    const std::int64_t s = std::int64_t(out) * out_scale_ / (QH * QB);
    return Score(std::clamp<std::int64_t>(s, -SCORE_KNOWN_WIN + 1, SCORE_KNOWN_WIN - 1));
}

template <int L1, int L2>
Score Network::forward(const Accumulator& acc, Color stm, int pieces) const {
    const int b = bucket(pieces, arch_.buckets), hb = arch_.version == 2 ? b : 0;
    Forward f;
    transform(acc, stm, f);
    hidden1<L1>(f, hb);
    hidden2<L1, L2>(f, hb);
    return output<L2>(f, b);
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

Network::Delta AccumulatorStack::delta(const Entry& e, Color view) {
    const Color us = e.us, them = ~us;
    const bool to_king = e.king || (bit(e.to) & promo_row(us));
    Network::Delta d;
    d.add = feature(view, us, to_king, e.to);
    d.nsub = 0;
    d.sub[d.nsub++] = feature(view, us, e.king, e.from);
    for (Bitboard b = e.captures; b;) {
        const int sq = pop_lsb(b);
        d.sub[d.nsub++] = feature(view, them, (e.captured_kings >> sq) & 1, sq);
    }
    return d;
}

void AccumulatorStack::compute(const Network& net, const Position& pos, int ply, Color view) {
    int k = ply;
    while (k > 0 && !entries_[k].computed[view]) --k;
    if (entries_[k].computed[view]) {
        // All the moves since then in one pass, in batches of up to 8.
        while (k < ply) {
            const int n = std::min(ply - k, 8);
            Network::Delta deltas[8];
            std::int16_t* dst[8];
            for (int i = 0; i < n; ++i) {
                Entry& e = entries_[k + 1 + i];
                deltas[i] = delta(e, view);
                dst[i] = e.acc.v[view];
                e.computed[view] = true;
            }
            net.update(entries_[k].acc.v[view], dst, deltas, n);
            k += n;
        }
    } else {
        net.refresh(entries_[ply].acc.v[view], pos, view);
        entries_[ply].computed[view] = true;
    }
}

void Network::profile(const std::vector<Position>& positions, std::ostream& out) const {
    const int n = int(positions.size());
    if (n == 0) return;
    // The layers at this net's widths.
    auto hidden1 = [&](Forward& f, int hb) { with_width(arch_.l1, [&](auto l1) { this->hidden1<l1()>(f, hb); }); };
    auto hidden2 = [&](Forward& f, int hb) {
        with_width(arch_.l1, [&](auto l1) {
            with_width(arch_.l2, [&](auto l2) { this->hidden2<l1(), l2()>(f, hb); });
        });
    };
    auto output = [&](const Forward& f, int b) {
        Score s = 0;
        with_width(arch_.l2, [&](auto l2) { s = this->output<l2()>(f, b); });
        return s;
    };
    // Inputs for every stage, prepared up front: accumulators, forward-pass activations and one random legal move
    // per position as feature changes for both points of view (as AccumulatorStack::delta builds them).
    std::vector<Accumulator> accs(n), next(n);
    std::vector<Forward> fw(n);
    std::vector<int> buckets(n), hidden_buckets(n);
    std::vector<std::array<Delta, 2>> moves(n);
    std::mt19937_64 rng(1);
    for (int i = 0; i < n; ++i) {
        const Position& pos = positions[i];
        buckets[i] = bucket(pos.piece_count(), arch_.buckets);
        hidden_buckets[i] = arch_.version == 2 ? buckets[i] : 0;
        refresh(accs[i], pos);
        transform(accs[i], pos.side_to_move(), fw[i]);
        hidden1(fw[i], hidden_buckets[i]);
        hidden2(fw[i], hidden_buckets[i]);
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) continue;
        const Move m = list[rng() % list.size()];
        const Color us = pos.side_to_move();
        const bool king = (pos.kings() >> m.from) & 1, to_king = king || (bit(m.to) & promo_row(us));
        for (Color view : {BLACK, WHITE}) {
            Delta& d = moves[i][view];
            d.add = feature(view, us, to_king, m.to);
            d.nsub = 0;
            d.sub[d.nsub++] = feature(view, us, king, m.from);
            for (Bitboard b = m.captures; b;) {
                const int sq = pop_lsb(b);
                d.sub[d.nsub++] = feature(view, ~us, (pos.kings() >> sq) & 1, sq);
            }
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
        for (Color view : {BLACK, WHITE}) {
            std::int16_t* dst = next[i].v[view];
            update(accs[i].v[view], &dst, &moves[i][view], 1);
        }
    });
    const double t_eval = time([&](int i) {
        sink += evaluate(accs[i], positions[i].side_to_move(), positions[i].piece_count());
    });
    const double t_transform = time([&](int i) { transform(accs[i], positions[i].side_to_move(), fw[i]); });
    const double t_hidden1 = time([&](int i) { hidden1(fw[i], hidden_buckets[i]); });
    const double t_hidden2 = time([&](int i) { hidden2(fw[i], hidden_buckets[i]); });
    const double t_output = time([&](int i) { sink += output(fw[i], buckets[i]); });

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
    out << std::fixed << std::setprecision(1) << "nnue profile: " << arch_.describe() << ", " << n
        << " positions, ns per call\n"
        << "  refresh (both views)          " << std::setw(7) << t_refresh << '\n'
        << "  update (one move, both views) " << std::setw(7) << t_update << '\n'
        << "  evaluate                      " << std::setw(7) << t_eval << '\n'
        << "    transform (SCReLU)          " << std::setw(7) << t_transform << '\n'
        << "    FT -> L1                    " << std::setw(7) << t_hidden1 << '\n'
        << "    L1 -> L2                    " << std::setw(7) << t_hidden2 << '\n'
        << "    output                      " << std::setw(7) << t_output << '\n'
        << "  FT output: " << density(&Forward::a, arch_.l1_inputs()) << '\n'
        << "  L1 output: " << density(&Forward::h1, arch_.l1) << '\n'
        << "  (checksum " << sink % 1000 << ")" << std::endl;
}

}  // namespace regicide::nnue
