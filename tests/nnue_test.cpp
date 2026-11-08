// NNUE checks on a random network: the engine's forward pass against a naive reference written from the file
// format, lazy incremental accumulators against a full refresh, colour-flip symmetry, header validation and
// loading through Evaluator::from_file.
//
//   nnue_test [version ft l1 l2 buckets]     (default: 1 1024 32 32 8)
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
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
std::vector<unsigned char> random_net(const nn::Arch& arch, std::uint64_t seed) {
    std::mt19937_64 rng(seed);
    auto uni = [&](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };
    std::vector<unsigned char> d{'R', 'G', 'N', 'N'};
    for (std::uint32_t v : {std::uint32_t(arch.version), std::uint32_t(nn::INPUTS), std::uint32_t(arch.ft),
                            std::uint32_t(arch.l1), std::uint32_t(arch.l2), std::uint32_t(arch.buckets),
                            std::uint32_t(nn::ACTIVATION), std::uint32_t(nn::QA), std::uint32_t(nn::QB), 400u})
        put(d, v);
    const int hb = arch.hidden_buckets(), n1 = arch.l1_inputs();
    for (int i = 0; i < arch.ft; ++i) put(d, std::int16_t(uni(-40, 160)));
    for (int i = 0; i < nn::INPUTS * arch.ft; ++i) put(d, std::int16_t(uni(-90, 90)));
    for (int i = 0; i < hb * arch.l1; ++i) put(d, std::int32_t(uni(-30000, 30000)));
    for (int i = 0; i < hb * arch.l1 * n1; ++i) put(d, std::int8_t(uni(-127, 127)));
    for (int i = 0; i < hb * arch.l2; ++i) put(d, std::int32_t(uni(-6000, 8000)));
    for (int i = 0; i < hb * arch.l2 * arch.l1; ++i) put(d, std::int8_t(uni(-127, 127)));
    for (int i = 0; i < arch.buckets; ++i) put(d, std::int32_t(uni(-50000, 50000)));
    for (int i = 0; i < arch.buckets * arch.l2; ++i) put(d, std::int16_t(uni(-400, 400)));
    return d;
}

// Straightforward evaluation from the raw file bytes, with its own orientation and 64-bit arithmetic.
struct Reference {
    const unsigned char* p;
    nn::Arch arch;
    template <class T>
    T at(std::size_t offset, std::size_t i) const {
        T v;
        std::memcpy(&v, p + offset + i * sizeof(T), sizeof(T));
        return v;
    }
    int eval(const Position& pos) const {
        using std::size_t;
        const int ft = arch.ft, l1 = arch.l1, l2 = arch.l2, buckets = arch.buckets;
        const bool v2 = arch.version == 2;
        const size_t hb = v2 ? buckets : 1, n1 = v2 ? 2 * ft : ft;
        const size_t ftb = nn::HEADER_BYTES, ftw = ftb + 2 * ft, l1b = ftw + 2 * nn::INPUTS * ft,
                     l1w = l1b + 4 * hb * l1, l2b = l1w + hb * l1 * n1, l2w = l2b + 4 * hb * l2,
                     ob = l2w + hb * l2 * l1, ow = ob + 4 * buckets;
        check(ow + 2 * size_t(buckets) * l2 == arch.file_bytes(), "reference layout vs Arch::file_bytes");
        std::uint32_t out_scale;
        std::memcpy(&out_scale, p + 40, 4);

        // SCReLU of the feature transformer seen from `view`.
        auto transformed = [&](Color view) {
            std::vector<int> active;
            for (int sq = 0; sq < 32; ++sq) {
                const Bitboard b = bit(sq);
                if (!(pos.occupied() & b)) continue;
                const bool ours = pos.pieces(view) & b, king = pos.kings() & b;
                const int osq = view == BLACK ? 31 - sq : sq;  // black's view: rotated 180 degrees
                active.push_back(((ours ? 0 : 2) + (king ? 1 : 0)) * 32 + osq);
            }
            std::vector<long long> a(ft);
            for (int i = 0; i < ft; ++i) {
                long long acc = at<std::int16_t>(ftb, i);
                for (int f : active) acc += at<std::int16_t>(ftw, size_t(f) * ft + i);
                const long long c = std::clamp(acc, 0LL, 255LL);
                a[i] = c * c / 512;
            }
            return a;
        };
        const Color stm = pos.side_to_move();
        std::vector<long long> a = transformed(stm);
        if (v2) {
            const auto opp = transformed(~stm);
            a.insert(a.end(), opp.begin(), opp.end());
        }
        const int n = pos.piece_count();
        const int bucket = std::min((n - 1) * buckets / 24, buckets - 1);
        const size_t hbucket = v2 ? bucket : 0;
        auto layer = [&](const std::vector<long long>& in, size_t bias, size_t weight, int m) {
            std::vector<long long> out(m);
            for (int j = 0; j < m; ++j) {
                long long s = at<std::int32_t>(bias, hbucket * m + j);
                for (size_t i = 0; i < in.size(); ++i)
                    s += in[i] * at<std::int8_t>(weight, (hbucket * m + j) * in.size() + i);
                const long long q = s >= 0 ? s / 64 : -((-s + 63) / 64);  // floor division
                out[j] = std::clamp(q, 0LL, 127LL);
            }
            return out;
        };
        const auto h1 = layer(a, l1b, l1w, l1);
        const auto h2 = layer(h1, l2b, l2w, l2);
        long long out = at<std::int32_t>(ob, bucket);
        for (int j = 0; j < l2; ++j) out += h2[j] * at<std::int16_t>(ow, size_t(bucket) * l2 + j);
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

int main(int argc, char** argv) {
    nn::Arch arch;
    if (argc == 6) {
        arch.version = std::atoi(argv[1]);
        arch.ft = std::atoi(argv[2]);
        arch.l1 = std::atoi(argv[3]);
        arch.l2 = std::atoi(argv[4]);
        arch.buckets = std::atoi(argv[5]);
    } else if (argc != 1) {
        std::cerr << "usage: nnue_test [version ft l1 l2 buckets]\n";
        return 2;
    }
    const std::vector<unsigned char> bytes = random_net(arch, 42);
    std::string error;
    const auto net = nn::Network::load(bytes.data(), bytes.size(), &error);
    if (!net) {
        std::cerr << "random net rejected: " << error << '\n';
        return 1;
    }
    std::cout << "net: " << net->arch().describe() << ", " << bytes.size() << " bytes\n";
    check(net->both_views() == (arch.version == 2), "both_views");
    const Reference ref{bytes.data(), arch};

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

    // Header validation: each bad net is rejected with a reason.
    auto rejected = [&](std::vector<unsigned char> d, const std::string& what) {
        std::string why;
        const bool ok = nn::Network::load(d.data(), d.size(), &why) == nullptr && !why.empty();
        check(ok, what + " accepted");
        if (ok && failures == 0 && std::getenv("NNUE_TEST_VERBOSE")) std::cout << "  " << what << ": " << why << '\n';
    };
    auto with_field = [&](int field, std::uint32_t value) {
        auto d = bytes;
        std::memcpy(d.data() + 4 + 4 * field, &value, 4);
        return d;
    };
    rejected(with_field(0, 3), "version 3");
    rejected(with_field(0, arch.version == 1 ? 2 : 1), "other version, same sizes");
    rejected(with_field(1, nn::INPUTS + 1), "wrong input count");
    rejected(with_field(2, std::uint32_t(arch.ft) ^ 1), "FT width not a multiple of the tile");
    rejected(with_field(2, nn::MAX_FT + nn::FT_TILE), "FT width over the build's maximum");
    rejected(with_field(2, 0), "FT width 0");
    rejected(with_field(3, 48), "unsupported L1 width");
    rejected(with_field(4, 8), "unsupported L2 width");
    rejected(with_field(5, std::uint32_t(arch.buckets) + 1), "wrong bucket count");
    rejected(with_field(5, 0), "no buckets");
    rejected(with_field(6, 2), "other activation");
    rejected(with_field(7, nn::QA + 1), "other QA");
    rejected(with_field(8, nn::QB * 2), "other QB");
    auto patched = bytes;
    patched.pop_back();
    rejected(patched, "truncated net");
    patched = bytes;
    patched.push_back(0);
    rejected(patched, "net with trailing bytes");
    patched = bytes;
    patched[3] = 'X';
    rejected(patched, "bad magic");
    rejected(std::vector<unsigned char>(bytes.begin(), bytes.begin() + 20), "header only");
    {
        std::string why;
        const auto d = with_field(3, 48);
        nn::Network::load(d.data(), d.size(), &why);
        check(why.find("l1") != std::string::npos, "reason names the bad field: " + why);
    }

    // Loading through the evaluator: RGNN and RGEV are told apart by their headers.
    const auto path = std::filesystem::temp_directory_path() /
                      ("regicide_nnue_test_v" + std::to_string(arch.version) + "_" + std::to_string(arch.ft) + "_" +
                       std::to_string(arch.l1) + "_" + std::to_string(arch.l2) + "_" + std::to_string(arch.buckets) +
                       ".bin");
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
