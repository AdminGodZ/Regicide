// nn_check: verify the ported GuiNN 2.06 neural-net inference.
//
// For a few thousand positions from deterministic random games it compares
//   1. the engine's full evaluation path        (ConvertToInputValues + GetSum, SIMD code),
//   2. the engine's incremental path             (IncrementalUpdate + GetSumIncremental, as in search),
//   3. an independent scalar reference written here from the network definition, with the
//      integer semantics of the original AVX2 code (wrapping int16 first layer, int32
//      accumulation, arithmetic >> 8, saturating pack to int16, ReLU).
// Any difference is an error. With an output file it also writes "fen phase value" lines, so
// the arm64/NEON build can be diffed against the original x86 AVX2 code (run under Rosetta).
//
// usage: nn_check <dir containing Nets206.gnn> [values.txt] [positions]

#include <unistd.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "engine.h"

namespace {

struct Rng {
    std::uint64_t s;
    std::uint32_t next() {
        s ^= s << 13;
        s ^= s >> 7;
        s ^= s << 17;
        return std::uint32_t(s >> 32);
    }
};

struct Counters {
    long saturated = 0;  // hidden-layer values clipped by the 32->16 bit saturating pack
    long overflow = 0;   // int32 accumulator overflow (wraps in SIMD; never expected)
};

// Network inputs, rebuilt here from the net definition: 32 white king squares, 28 white man
// squares (no white back rank), the same for black, and one side-to-move input.
std::vector<int> active_inputs(const Board& b) {
    std::vector<int> in;
    for (int c : {BLACK, WHITE}) {
        const int start = (c == WHITE) ? 0 : 60;
        for (int sq = 0; sq < 32; ++sq) {
            if (!(b.Bitboards.P[c] & (1u << sq))) continue;
            if (b.Bitboards.K & (1u << sq)) in.push_back(start + 28 + sq);
            else in.push_back(c == BLACK ? start + sq : start + sq - 4);
        }
    }
    if (b.sideToMove == BLACK) in.push_back(120);
    return in;
}

int reference_eval(const CheckersNet& net, const Board& b, Counters& cnt) {
    const NeuralNetwork<nnInt_t>& nn = net.network;

    // Layer 0, sparse: weights stored input-major. int16 adds wrap; the sum mod 2^16 does not
    // depend on the order, so accumulate wide and truncate once.
    const NetworkTransform<nnInt_t>* t0 = nn.GetTransform(0);
    std::vector<std::int32_t> act(t0->outputCount);
    const std::vector<int> inputs = active_inputs(b);
    for (std::uint32_t o = 0; o < t0->outputCount; ++o) {
        std::int64_t v = t0->Weights[t0->biasStart + o];
        for (int i : inputs) v += t0->Weights[t0->weightStart + std::uint32_t(i) * t0->outputCount + o];
        std::int16_t w = std::int16_t(std::uint16_t(std::uint64_t(v)));
        act[o] = std::max<std::int32_t>(w, 0);  // ReLU
    }

    // Dense layers: weights stored output-major, bias << 8, int32 accumulation, >> 8.
    for (int l = 1; l < nn.LayerCount(); ++l) {
        const NetworkTransform<nnInt_t>* t = nn.GetTransform(l);
        std::vector<std::int32_t> next(t->outputCount);
        for (std::uint32_t o = 0; o < t->outputCount; ++o) {
            std::int64_t acc = std::int64_t(t->Weights[t->biasStart + o]) * 256;
            for (std::uint32_t i = 0; i < t->inputCount; ++i)
                acc += std::int64_t(t->Weights[t->weightStart + o * t->inputCount + i]) * act[i];
            if (acc < INT32_MIN || acc > INT32_MAX) ++cnt.overflow;
            std::int32_t a32 = std::int32_t(std::uint32_t(std::uint64_t(acc)));
            std::int32_t shifted = a32 >> 8;
            if (t->activationType == eActivation::RELU) {
                if (shifted > 32767) ++cnt.saturated;
                next[o] = std::clamp(shifted, 0, 32767);  // packs_epi32 saturation, then max(0)
            } else {
                next[o] = std::clamp(shifted, -kFixedMax, kFixedMax);
            }
        }
        act = std::move(next);
    }
    return act[0];
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: nn_check <dir with Nets206.gnn> [values.txt] [positions]\n");
        return 2;
    }
    if (chdir(argv[1]) != 0) {
        std::perror(argv[1]);
        return 2;
    }
    FILE* out = argc > 2 ? std::fopen(argv[2], "w") : nullptr;
    const long target = argc > 3 ? std::atol(argv[3]) : 4000;

    InitBitTables();
    if (InitializeNeuralNets() != 4) {
        std::fprintf(stderr, "could not load the 4 nets (Nets206.gnn)\n");
        return 2;
    }
    std::printf("%s: loaded %zu nets\n", g_VersionName, engine.evalNets.size());

    nnInt_t* values = AlignedAllocUtil<nnInt_t>(kMaxEvalNetValues, 64);
    nnInt_t* prevLayer = AlignedAllocUtil<nnInt_t>(kMaxValuesInLayer, 64);
    nnInt_t* curLayer = AlignedAllocUtil<nnInt_t>(kMaxValuesInLayer, 64);

    Rng rng{0x9E3779B97F4A7C15ull};
    Counters cnt;
    long checked = 0, mismatchRef = 0, mismatchIncr = 0, perPhase[4] = {0, 0, 0, 0};
    long games = 0;

    while (checked < target) {
        ++games;
        Board board = Board::StartPosition();
        int prevNet = -1;
        for (int ply = 0; ply < 300 && checked < target; ++ply) {
            const int netIdx = (int)CheckersNet::GetGamePhase(board);
            CheckersNet* net = engine.evalNets[netIdx];
            if (board.Bitboards.GetCheckers() != 0) {
                // 1. full evaluation
                int full = net->GetSum(board, values);
                // 2. incremental first layer, kept up to date along the game like the search does
                if (netIdx != prevNet) net->ComputeFirstLayerValues(board, values, curLayer);
                int incr = net->GetSumIncremental(curLayer, values);
                // 3. scalar reference
                int ref = reference_eval(*net, board, cnt);

                if (full != ref) {
                    if (++mismatchRef <= 10)
                        std::printf("MISMATCH ref %s: simd %d reference %d\n", board.ToString().c_str(), full, ref);
                }
                if (incr != full) {
                    if (++mismatchIncr <= 10)
                        std::printf("MISMATCH incremental %s: full %d incremental %d\n", board.ToString().c_str(), full, incr);
                }
                if (out) std::fprintf(out, "%s %d %d\n", board.ToString().c_str(), netIdx, full);
                ++perPhase[netIdx];
                ++checked;
            }

            MoveList moves;
            moves.FindMoves(board);
            if (moves.numMoves == 0) break;
            Move m = moves.moves[rng.next() % moves.numMoves];

            // Carry the incremental first-layer values across the move (search.cpp DoMove()).
            Board before = board;
            board.DoMove(m);
            const int nextNet = (int)CheckersNet::GetGamePhase(board);
            if (board.Bitboards.GetCheckers() != 0 && netIdx == nextNet && prevNet == netIdx) {
                std::copy(curLayer, curLayer + kMaxValuesInLayer, prevLayer);
                engine.evalNets[nextNet]->IncrementalUpdate(m, before, prevLayer);
                std::copy(prevLayer, prevLayer + kMaxValuesInLayer, curLayer);
                prevNet = nextNet;
            } else if (board.Bitboards.GetCheckers() != 0) {
                engine.evalNets[nextNet]->ComputeFirstLayerValues(board, values, curLayer);
                prevNet = nextNet;
            } else {
                prevNet = -1;
            }
        }
    }
    if (out) std::fclose(out);

    std::printf("positions: %ld from %ld random games (early %ld, mid %ld, end %ld, late end %ld)\n", checked, games,
                perPhase[0], perPhase[1], perPhase[2], perPhase[3]);
    std::printf("hidden values saturated by the 16-bit pack: %ld, int32 overflows: %ld\n", cnt.saturated, cnt.overflow);
    std::printf("simd vs scalar reference mismatches: %ld\n", mismatchRef);
    std::printf("incremental vs full mismatches: %ld\n", mismatchIncr);
    bool ok = mismatchRef == 0 && mismatchIncr == 0 && perPhase[0] && perPhase[1] && perPhase[2] && perPhase[3];
    std::printf("%s\n", ok ? "NN CHECK PASSED" : "NN CHECK FAILED");
    return ok ? 0 : 1;
}
