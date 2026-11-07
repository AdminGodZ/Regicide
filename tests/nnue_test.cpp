// NNUE checks on a random network: the engine's forward pass against a naive reference written from the file
// format, lazy incremental accumulators against a full refresh, colour-flip symmetry, header validation and
// loading through Evaluator::from_file.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "eval.hpp"
#include "movegen.hpp"
#include "nnue.hpp"

using namespace regicide;
namespace nn = regicide::nnue;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok && ++failures <= 20) std::cerr << "FAIL: " << what << '\n';
}

template <class T>
void put(std::vector<unsigned char>& out, T v) {
    unsigned char b[sizeof(T)];
    std::memcpy(b, &v, sizeof(T));
    out.insert(out.end(), b, b + sizeof(T));
}

// A random net in the RGNN layout, with weights in ranges that exercise both sides of every clamp.
std::vector<unsigned char> random_net(std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    auto uni = [&](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };
    std::vector<unsigned char> d{'R', 'G', 'N', 'N'};
    for (std::uint32_t v : {nn::VERSION, std::uint32_t(nn::INPUTS), std::uint32_t(nn::FT), std::uint32_t(nn::L1),
                            std::uint32_t(nn::L2), std::uint32_t(nn::BUCKETS), std::uint32_t(nn::ACTIVATION),
                            std::uint32_t(nn::QA), std::uint32_t(nn::QB), 400u})
        put(d, v);
    for (int i = 0; i < nn::FT; ++i) put(d, std::int16_t(uni(-40, 160)));
    for (int i = 0; i < nn::INPUTS * nn::FT; ++i) put(d, std::int16_t(uni(-90, 90)));
    for (int i = 0; i < nn::L1; ++i) put(d, std::int32_t(uni(-30000, 30000)));
    for (int i = 0; i < nn::L1 * nn::FT; ++i) put(d, std::int8_t(uni(-127, 127)));
    for (int i = 0; i < nn::L2; ++i) put(d, std::int32_t(uni(-6000, 8000)));
    for (int i = 0; i < nn::L2 * nn::L1; ++i) put(d, std::int8_t(uni(-127, 127)));
    for (int i = 0; i < nn::BUCKETS; ++i) put(d, std::int32_t(uni(-50000, 50000)));
    for (int i = 0; i < nn::BUCKETS * nn::L2; ++i) put(d, std::int16_t(uni(-400, 400)));
    return d;
}

// Straightforward evaluation from the raw file bytes, with its own orientation and 64-bit arithmetic.
struct Reference {
    const unsigned char* p;
    template <class T>
    T at(std::size_t offset, std::size_t i) const {
        T v;
        std::memcpy(&v, p + offset + i * sizeof(T), sizeof(T));
        return v;
    }
    int eval(const Position& pos) const {
        using std::size_t;
        const size_t ftb = nn::HEADER_BYTES, ftw = ftb + 2 * nn::FT, l1b = ftw + 2 * nn::INPUTS * nn::FT,
                     l1w = l1b + 4 * nn::L1, l2b = l1w + nn::L1 * nn::FT, l2w = l2b + 4 * nn::L2,
                     ob = l2w + nn::L2 * nn::L1, ow = ob + 4 * nn::BUCKETS;
        std::uint32_t out_scale;
        std::memcpy(&out_scale, p + 40, 4);

        const Color stm = pos.side_to_move();
        std::vector<int> active;
        for (int sq = 0; sq < 32; ++sq) {
            const Bitboard b = bit(sq);
            if (!(pos.occupied() & b)) continue;
            const bool ours = pos.pieces(stm) & b, king = pos.kings() & b;
            const int osq = stm == BLACK ? 31 - sq : sq;  // black's view: rotated 180 degrees
            active.push_back(((ours ? 0 : 2) + (king ? 1 : 0)) * 32 + osq);
        }
        std::vector<long long> a(nn::FT);
        for (int i = 0; i < nn::FT; ++i) {
            long long acc = at<std::int16_t>(ftb, i);
            for (int f : active) acc += at<std::int16_t>(ftw, size_t(f) * nn::FT + i);
            const long long c = std::clamp(acc, 0LL, 255LL);
            a[i] = c * c / 512;
        }
        auto layer = [&](const std::vector<long long>& in, size_t bias, size_t weight, int n) {
            std::vector<long long> out(n);
            for (int j = 0; j < n; ++j) {
                long long s = at<std::int32_t>(bias, j);
                for (size_t i = 0; i < in.size(); ++i) s += in[i] * at<std::int8_t>(weight, j * in.size() + i);
                const long long q = s >= 0 ? s / 64 : -((-s + 63) / 64);  // floor division
                out[j] = std::clamp(q, 0LL, 127LL);
            }
            return out;
        };
        const auto h1 = layer(a, l1b, l1w, nn::L1);
        const auto h2 = layer(h1, l2b, l2w, nn::L2);
        const int n = pos.piece_count();
        const int bucket = std::min((n - 1) * nn::BUCKETS / 24, nn::BUCKETS - 1);
        long long out = at<std::int32_t>(ob, bucket);
        for (int j = 0; j < nn::L2; ++j) out += h2[j] * at<std::int16_t>(ow, size_t(bucket) * nn::L2 + j);
        const long long s = out * out_scale / (127 * 64);
        return int(std::clamp(s, -14999LL, 14999LL));
    }
};

Bitboard rotate(Bitboard b) {
    Bitboard r = 0;
    for (int sq = 0; sq < 32; ++sq)
        if (b & bit(sq)) r |= bit(31 - sq);
    return r;
}

struct Walker {
    const nn::Network& net;
    const Reference& ref;
    nn::AccumulatorStack& stack;
    std::mt19937_64& rng;
    long evaluated = 0, distinct_scores = 0;
    int last_score = 0;

    // Random tree walk: evaluate some nodes, visit one or two children, so the lazy stack sees fresh chains,
    // re-used entries and siblings that overwrite an earlier path.
    void walk(const Position& pos, int ply, int depth) {
        if (rng() % 2 == 0) {
            const int expect = ref.eval(pos);
            const int got = stack.evaluate(net, pos, ply);
            check(got == expect, "incremental " + std::to_string(got) + " vs reference " + std::to_string(expect) +
                                     " at " + pos.fen());
            check(net.evaluate(pos) == expect, "refresh vs reference at " + pos.fen());
            distinct_scores += got != last_score;
            last_score = got;
            ++evaluated;
        }
        if (depth == 0) return;
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) return;
        const int children = 1 + int(rng() % 2);
        for (int c = 0; c < children; ++c) {
            const Move m = list[rng() % list.size()];
            Position child = pos;
            child.make_move(m);
            stack.push(ply + 1, pos, m);
            walk(child, ply + 1, depth - 1);
        }
    }
};

}  // namespace

int main() {
    const std::vector<unsigned char> bytes = random_net(42);
    std::string error;
    const auto net = nn::Network::load(bytes.data(), bytes.size(), &error);
    if (!net) {
        std::cerr << "random net rejected: " << error << '\n';
        return 1;
    }
    const Reference ref{bytes.data()};

    // Forward pass, incremental updates and symmetry over random playouts.
    std::mt19937_64 rng(7);
    auto stack = std::make_unique<nn::AccumulatorStack>();
    Walker walker{*net, ref, *stack, rng};
    long symmetric = 0;
    for (int game = 0; game < 300; ++game) {
        Position pos;
        const int plies = int(rng() % 90);
        for (int i = 0; i < plies; ++i) {
            MoveList list;
            generate_moves(pos, list);
            if (list.empty()) break;
            pos.make_move(list[rng() % list.size()]);
        }
        stack->reset(*net, pos);
        walker.walk(pos, 0, 12);

        Position flipped;
        flipped.set(rotate(pos.pieces(WHITE)), rotate(pos.pieces(BLACK)), rotate(pos.kings()), ~pos.side_to_move());
        check(net->evaluate(flipped) == net->evaluate(pos), "colour-flip symmetry at " + pos.fen());
        ++symmetric;
    }
    std::cout << "forward/incremental: " << walker.evaluated << " positions (" << walker.distinct_scores
              << " score changes), symmetry: " << symmetric << " positions\n";
    check(walker.evaluated > 1000 && walker.distinct_scores > walker.evaluated / 2, "random net too degenerate");

    // Header validation.
    auto rejected = [&](std::vector<unsigned char> d) { return nn::Network::load(d.data(), d.size()) == nullptr; };
    auto patched = bytes;
    patched[8] = std::uint8_t(nn::INPUTS + 1);
    check(rejected(patched), "wrong input count accepted");
    patched = bytes;
    patched[12] ^= 1;
    check(rejected(patched), "wrong FT width accepted");
    patched = bytes;
    patched.pop_back();
    check(rejected(patched), "truncated net accepted");
    patched = bytes;
    patched[3] = 'X';
    check(rejected(patched), "bad magic accepted");

    // Loading through the evaluator: RGNN and RGEV are told apart by their headers.
    const auto path = std::filesystem::temp_directory_path() / "regicide_nnue_test.bin";
    if (std::FILE* f = std::fopen(path.string().c_str(), "wb")) {
        std::fwrite(bytes.data(), 1, bytes.size(), f);
        std::fclose(f);
    }
    const auto ev = Evaluator::from_file(path.string());
    std::filesystem::remove(path);
    check(ev && ev->network(), "Evaluator::from_file did not load the net");
    if (ev) check((*ev)(Position()) == ref.eval(Position()), "Evaluator dispatch to the net");
    const auto linear = Evaluator::from_file(REGICIDE_SOURCE_DIR "/src/weights/eval.bin");
    check(linear && linear->trained() && !linear->network(), "RGEV weights through from_file");

    // The built-in evaluator uses the embedded net exactly when src/weights/net.bin is non-empty.
    const bool embedded = REGICIDE_NET_BYTES > 0;
    check(bool(Evaluator::builtin()->network()) == embedded,
          embedded ? "embedded net.bin was rejected" : "built-in evaluator has a net without net.bin");

    if (failures) {
        std::cerr << failures << " failures\n";
        return 1;
    }
    std::cout << "nnue ok\n";
    return 0;
}
