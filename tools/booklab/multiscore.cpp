// booklab_multiscore (WP12): score every legal move of a position with the same fixed node budget, book off, a
// cleared hash for every move, and the score of the last COMPLETED iteration only.
//
//   booklab_multiscore NODES THREADS [EGDB_DIR [HASH_MB [BOOK]]] < points.tsv
// stdin, one point per line: ID <TAB> FEN
// stdout, one line per legal move: ID <TAB> move <TAB> completed score (side to move's view; "none" if no iteration
// completed) <TAB> depth (SearchResult::depth) <TAB> nodes <TAB> partial (1 = the last iteration was cut) <TAB>
// avoided (1 = on BOOK's avoid list; "-" without BOOK). NODES 0: no search, only the moves and the avoid flags
// (score "-"). The search itself never uses the book or its avoid list.
// Points are distributed over THREADS workers (one Engine each); lines of different points may interleave.
#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "book.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "movetext.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: booklab_multiscore NODES THREADS [EGDB_DIR [HASH_MB [BOOK]]] < points.tsv\n";
        return 2;
    }
    const std::uint64_t nodes = std::stoull(argv[1]);
    const int threads = std::stoi(argv[2]);
    int egdb_pieces = 0;
    if (argc > 3) {
        if (!egdb::init(argv[3], 2048)) {
            std::cerr << "no endgame database in " << argv[3] << '\n';
            return 1;
        }
        egdb_pieces = egdb::max_pieces();
    }
    const std::size_t hash_mb = argc > 4 ? std::stoul(argv[4]) : 128;
    Book avoid_book;
    const bool have_book = argc > 5;
    if (have_book && !avoid_book.load(argv[5])) {
        std::cerr << "cannot load " << argv[5] << '\n';
        return 1;
    }
    std::vector<std::pair<std::string, std::string>> points;
    for (std::string line; std::getline(std::cin, line);) {
        const auto t = line.find('\t');
        if (t != std::string::npos) points.push_back({line.substr(0, t), line.substr(t + 1)});
    }
    std::atomic<std::size_t> next{0};
    std::mutex out_mu;
    std::vector<std::thread> pool;
    for (int w = 0; w < threads; ++w)
        pool.emplace_back([&] {
            Engine engine;
            engine.set_hash_mb(hash_mb);
            engine.set_book(nullptr);
            engine.set_avoid(nullptr);
            engine.set_egdb_pieces(egdb_pieces);
            for (std::size_t i; (i = next++) < points.size();) {
                Position pos;
                if (!pos.set_fen(points[i].second)) {
                    std::lock_guard l(out_mu);
                    std::cout << points[i].first << "\tERR\tFEN\n";
                    continue;
                }
                MoveList list;
                generate_moves(pos, list);
                std::ostringstream rows;
                for (const Move& m : list) {
                    const std::string av = have_book ? (avoid_book.avoids(pos, m) ? "1" : "0") : "-";
                    if (nodes == 0) {
                        rows << points[i].first << '\t' << booklab::move_text(pos, m) << "\t-\t0\t0\t0\t" << av << '\n';
                        continue;
                    }
                    engine.new_game();  // cleared hash and history for every move
                    engine.set_position(pos);
                    Limits lim;
                    lim.nodes = nodes;
                    lim.use_book = false;
                    lim.searchmoves = {m};
                    const SearchResult r = engine.go(lim);
                    rows << points[i].first << '\t' << booklab::move_text(pos, m) << '\t'
                         << (r.completed_score == -SCORE_INF ? std::string("none") : std::to_string(r.completed_score))
                         << '\t' << r.depth << '\t' << r.nodes << '\t' << (r.partial ? 1 : 0) << '\t' << av << '\n';
                }
                std::lock_guard l(out_mu);
                std::cout << rows.str() << std::flush;
            }
        });
    for (auto& t : pool) t.join();
    return 0;
}
