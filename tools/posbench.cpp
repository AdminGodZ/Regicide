// Fixed-limit searches over a list of positions: nodes per second, depth reached and time to depth.
//
//   posbench --fens mid.txt --count 50 --threads 4 --depth 18 --params "hash_first_db=90" --egdb ~/egdb
//
// Each position is searched from a cleared hash table (new_game). Prints one line per position with -v and totals.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "egdb/egdb.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    std::string fens, params;
    int count = 0, threads = 1;
    std::size_t hash_mb = 64;
    bool verbose = false;
    Limits limits;
    int egdb_pieces = 0;
    int ttd = 0;  // time to depth: search without a depth limit and stop when the main thread completes this depth
    for (int i = 1; i < argc; ++i) {
        std::string k = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (k == "--fens") fens = next();
        else if (k == "--count") count = std::stoi(next());
        else if (k == "--threads") threads = std::stoi(next());
        else if (k == "--hash") hash_mb = std::stoull(next());
        else if (k == "--params") params = next();
        else if (k == "--depth") limits.depth = std::stoi(next());
        else if (k == "--ttd") ttd = std::stoi(next());
        else if (k == "--movetime") limits.movetime_ms = std::stoi(next());
        else if (k == "--nodes") limits.nodes = std::stoull(next());
        else if (k == "--egdb") {
            if (!egdb::init(next(), 4096)) {
                std::cerr << "cannot load egdb\n";
                return 1;
            }
            egdb_pieces = egdb::max_pieces();
        } else if (k == "-v") verbose = true;
        else {
            std::cerr << "unknown option " << k << '\n';
            return 2;
        }
    }
    std::vector<Position> list;
    std::FILE* f = std::fopen(fens.c_str(), "r");
    if (!f) return 1;
    char line[512];
    while (std::fgets(line, sizeof line, f)) {
        std::string fen = line;
        fen = fen.substr(0, fen.find_first_of(" \t\r\n"));
        Position p;
        if (!fen.empty() && p.set_fen(fen)) list.push_back(p);
        if (count && int(list.size()) >= count) break;
    }
    std::fclose(f);

    Engine e;
    e.set_hash_mb(hash_mb);
    e.set_threads(threads);
    e.set_egdb_pieces(egdb_pieces);
    if (!e.params.set_all(params)) {
        std::cerr << "bad params\n";
        return 2;
    }
    std::uint64_t nodes = 0, tbhits = 0;
    std::int64_t ms = 0;
    long depth_sum = 0;
    for (std::size_t i = 0; i < list.size(); ++i) {
        e.new_game();
        e.set_position(list[i]);
        std::uint64_t tb = 0;
        bool reached = false;
        SearchInfo snap;  // --ttd: the report of the iteration that completed the target depth
        e.set_stop_poll([&] { return reached; });
        SearchResult r = e.go(limits, [&](const SearchInfo& info) {
            if (reached) return;  // later work (the stop is noticed up to 1,024 nodes late) is not reported
            tb = info.tbhits;
            if (ttd && info.depth >= ttd) {
                reached = true;
                snap = info;
            }
        });
        if (reached) {
            // Every reported field comes from the same completed-iteration snapshot.
            r.nodes = snap.nodes;
            r.time_ms = snap.time_ms;
            r.depth = snap.depth;
            r.score = snap.score;
            tb = snap.tbhits;
        }
        nodes += r.nodes;
        ms += r.time_ms;
        tbhits += tb;
        depth_sum += r.depth;
        if (verbose)
            std::printf("%zu %s depth %d score %d nodes %llu ms %lld\n", i, list[i].fen().c_str(), r.depth, r.score,
                        (unsigned long long)r.nodes, (long long)r.time_ms);
    }
    std::printf("positions %zu threads %d params \"%s\": nodes %llu ms %lld nps %.0f mean_depth %.2f mean_ms %.1f "
                "tbhits %llu\n",
                list.size(), threads, params.c_str(), (unsigned long long)nodes, (long long)ms,
                ms ? nodes * 1000.0 / ms : 0.0, double(depth_sum) / list.size(), double(ms) / list.size(),
                (unsigned long long)tbhits);
    return 0;
}
