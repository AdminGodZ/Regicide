// Fixed-limit searches over a list of positions: nodes per second, depth reached and time to depth.
//
//   posbench --fens mid.txt --count 50 --threads 4 --depth 18 --params "hash_first_db=90" --egdb ~/egdb
//
// Each position is searched from a cleared hash table (new_game). Prints one line per position with -v and totals.
#include <array>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    std::string fens, params, tsv;  // --tsv: tab file with columns fen and moves_from_start (Book Lab wp14)
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
        else if (k == "--tsv") tsv = next();
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
    std::vector<std::vector<Position>> hist;  // --tsv: game history before each position (repetitions)
    if (!tsv.empty()) {
        std::ifstream in(tsv);
        std::string row;
        int cf = -1, cm = -1;
        while (std::getline(in, row)) {
            std::vector<std::string> col;
            std::stringstream ls(row);
            for (std::string c; std::getline(ls, c, '\t');) col.push_back(c);
            if (cf < 0) {
                for (std::size_t j = 0; j < col.size(); ++j) {
                    if (col[j] == "fen") cf = int(j);
                    if (col[j] == "moves_from_start") cm = int(j);
                }
                if (cf < 0 || cm < 0) {
                    std::cerr << "tsv needs columns fen and moves_from_start\n";
                    return 2;
                }
                continue;
            }
            Position p;
            std::vector<Position> h;
            std::stringstream ms(col[std::size_t(cm)]);
            for (std::string t; ms >> t;) {
                Move m;
                if (!parse_move(p, t, m)) {
                    std::cerr << "bad move " << t << '\n';
                    return 2;
                }
                h.push_back(p);
                p.make_move(m);
            }
            Position want;
            if (!want.set_fen(col[std::size_t(cf)]) || want.fen() != p.fen()) {
                std::cerr << "replay does not reach " << col[std::size_t(cf)] << " (got " << p.fen() << ")\n";
                return 2;
            }
            list.push_back(p);
            hist.push_back(std::move(h));
            if (count && int(list.size()) >= count) break;
        }
    }
    std::FILE* f = tsv.empty() ? std::fopen(fens.c_str(), "r") : nullptr;
    if (!f && tsv.empty()) return 1;
    char line[512];
    while (f && std::fgets(line, sizeof line, f)) {
        std::string fen = line;
        fen = fen.substr(0, fen.find_first_of(" \t\r\n"));
        Position p;
        if (!fen.empty() && p.set_fen(fen)) list.push_back(p);
        if (count && int(list.size()) >= count) break;
    }
    if (f) std::fclose(f);

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
    std::array<std::uint64_t, 6> qs{};
    for (std::size_t i = 0; i < list.size(); ++i) {
        e.new_game();
        if (hist.empty()) e.set_position(list[i]);
        else e.set_position(list[i], hist[i]);
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
        for (int k = 0; k < 6; ++k) qs[k] += e.prune_stats()[k];
        nodes += r.nodes;
        ms += r.time_ms;
        tbhits += tb;
        depth_sum += r.depth;
        if (verbose)
        {
            MoveList legal;
            generate_moves(list[i], legal);
            std::printf("%zu %s depth %d score %d nodes %llu ms %lld move %s legal %zu\n", i, list[i].fen().c_str(),
                        r.depth, r.score, (unsigned long long)r.nodes, (long long)r.time_ms,
                        move_text(list[i], r.best).c_str(), legal.size());
        }
    }
    std::printf("positions %zu threads %d params \"%s\": nodes %llu ms %lld nps %.0f mean_depth %.2f mean_ms %.1f "
                "tbhits %llu\n",
                list.size(), threads, params.c_str(), (unsigned long long)nodes, (long long)ms,
                ms ? nodes * 1000.0 / ms : 0.0, double(depth_sum) / list.size(), double(ms) / list.size(),
                (unsigned long long)tbhits);
    std::printf("prune stats: lmp %llu kept %llu  fut %llu kept %llu  lmr %llu exempt %llu\n",
                (unsigned long long)qs[0], (unsigned long long)qs[1], (unsigned long long)qs[2],
                (unsigned long long)qs[3], (unsigned long long)qs[4], (unsigned long long)qs[5]);
    return 0;
}
