// Builds an unbalanced opening set (in the spirit of chess "UHO" books) for more sensitive self-play tests.
//
//   unbalanced --out uho.txt --count 1500 --nodes 2000000 --lo 50 --hi 130 --threads 18
//
// Candidates: a random position of the 3-ply set, followed by --min-plies..--max-plies more plies, each a 5k-node
// best move or, with probability --random %, a uniformly random legal move. A candidate is kept if it has at least
// --min-pieces pieces, the side to move has no capture (a quiet position), it is new (by hash key), and a fixed
// search of --nodes nodes scores it between --lo and --hi for either side. Output: one "FEN score" line per
// position, score from the side to move's point of view. A --pre-nodes search first drops candidates scored outside
// [lo - 30, hi + 60].
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <random>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

std::vector<Position> openings_at(int plies) {
    std::vector<Position> out;
    std::set<Key> seen;
    auto rec = [&](auto& self, const Position& p, int depth) -> void {
        if (depth == 0) {
            if (seen.insert(p.key()).second) out.push_back(p);
            return;
        }
        MoveList list;
        generate_moves(p, list);
        for (const Move& m : list) {
            Position c = p;
            c.make_move(m);
            self(self, c, depth - 1);
        }
    };
    rec(rec, Position(), plies);
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    std::string out_path = "uho.txt";
    int count = 1500, threads = 16, lo = 50, hi = 130, min_plies = 2, max_plies = 24, random_pct = 20;
    int min_pieces = 9, max_pieces = 24, seed = 1;
    std::uint64_t nodes = 2000000, walk_nodes = 5000, pre_nodes = 100000;
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--out") out_path = next();
        else if (k == "--count") count = std::stoi(next());
        else if (k == "--threads") threads = std::stoi(next());
        else if (k == "--nodes") nodes = std::stoull(next());
        else if (k == "--walk-nodes") walk_nodes = std::stoull(next());
        else if (k == "--pre-nodes") pre_nodes = std::stoull(next());
        else if (k == "--lo") lo = std::stoi(next());
        else if (k == "--hi") hi = std::stoi(next());
        else if (k == "--min-plies") min_plies = std::stoi(next());
        else if (k == "--max-plies") max_plies = std::stoi(next());
        else if (k == "--random") random_pct = std::stoi(next());
        else if (k == "--min-pieces") min_pieces = std::stoi(next());
        else if (k == "--max-pieces") max_pieces = std::stoi(next());
        else if (k == "--seed") seed = std::stoi(next());
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }
    const std::vector<Position> openings = openings_at(3);
    std::FILE* out = std::fopen(out_path.c_str(), "w");
    if (!out) return 1;

    std::mutex mu;
    std::set<Key> seen;
    std::atomic<int> kept{0}, tried{0};
    std::atomic<int> rej_pieces{0}, rej_capture{0}, rej_dup{0}, rej_low{0}, rej_high{0};

    auto worker = [&](int tid) {
        std::mt19937_64 rng(std::uint64_t(seed) * 1000003u + std::uint64_t(tid));
        Engine walk, judge;
        walk.set_hash_mb(16);
        judge.set_hash_mb(64);
        Limits wl, jl, pl;
        wl.nodes = walk_nodes;
        pl.nodes = pre_nodes;
        jl.nodes = nodes;
        while (kept.load() < count) {
            ++tried;
            Position pos = openings[rng() % openings.size()];
            std::vector<Position> hist;
            const int plies = min_plies + int(rng() % unsigned(max_plies - min_plies + 1));
            bool ok = true;
            walk.new_game();
            for (int p = 0; p < plies; ++p) {
                MoveList list;
                generate_moves(pos, list);
                if (list.empty()) {
                    ok = false;
                    break;
                }
                Move m;
                if (int(rng() % 100) < random_pct) {
                    m = list[rng() % list.size()];
                } else {
                    walk.set_position(pos, hist);
                    m = walk.go(wl).best;
                }
                hist.push_back(pos);
                pos.make_move(m);
            }
            if (!ok) continue;
            if (pos.piece_count() < min_pieces || pos.piece_count() > max_pieces) {
                ++rej_pieces;
                continue;
            }
            MoveList list;
            generate_moves(pos, list);
            if (list.empty() || list[0].is_capture()) {
                ++rej_capture;
                continue;
            }
            {
                std::lock_guard lock(mu);
                if (seen.contains(pos.key())) {
                    ++rej_dup;
                    continue;
                }
            }
            judge.new_game();
            judge.set_position(pos);
            if (pre_nodes) {  // cheap prefilter: skip clearly balanced or clearly decided candidates
                const int pa = std::abs(judge.go(pl).score);
                if (pa < lo - 30 || pa > hi + 60) {
                    ++(pa < lo - 30 ? rej_low : rej_high);
                    continue;
                }
            }
            const SearchResult r = judge.go(jl);
            const int a = std::abs(r.score);
            if (a < lo) {
                ++rej_low;
                continue;
            }
            if (a > hi) {
                ++rej_high;
                continue;
            }
            std::lock_guard lock(mu);
            if (!seen.insert(pos.key()).second || kept.load() >= count) continue;
            std::fprintf(out, "%s %d\n", pos.fen().c_str(), r.score);
            std::fflush(out);
            const int k = ++kept;
            if (k % 100 == 0)
                std::fprintf(stderr, "kept %d of %d tried (pieces %d, capture %d, dup %d, low %d, high %d)\n", k,
                             tried.load(), rej_pieces.load(), rej_capture.load(), rej_dup.load(), rej_low.load(),
                             rej_high.load());
        }
    };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t) pool.emplace_back(worker, t);
    for (auto& t : pool) t.join();
    std::fclose(out);
    std::fprintf(stderr, "done: kept %d of %d tried (pieces %d, capture %d, dup %d, low %d, high %d)\n", kept.load(),
                 tried.load(), rej_pieces.load(), rej_capture.load(), rej_dup.load(), rej_low.load(), rej_high.load());
    return 0;
}
