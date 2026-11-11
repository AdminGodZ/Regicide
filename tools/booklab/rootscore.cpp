// booklab_rootscore (WP14): our engine's root score at a position of a played game, with the game's history replayed
// (repetition), book off, the endgame database on, a cleared hash for every search, a fixed node budget; and the
// score of chosen root moves under the same conditions (searchmoves).
//
//   booklab_rootscore NODES THREADS [EGDB_DIR [HASH_MB]] < tasks.tsv
// stdin, one task per line: ID <TAB> start FEN (empty = initial position) <TAB> moves played before the position
// (space separated, PDN text, from the start FEN) <TAB> moves to score alone (comma separated, may be empty)
// stdout, one line per search: ID <TAB> "root" or the scored move <TAB> best move <TAB> score (side to move's view;
// the last completed iteration; "none" if none completed) <TAB> depth <TAB> nodes <TAB> partial (1 = the last
// iteration was cut) <TAB> pv (space separated)
// A task whose moves cannot be replayed (illegal or ambiguous) yields one line "ID <TAB> ERR <TAB> ply <TAB> move".
// Tasks are distributed over THREADS workers (one Engine each); lines of different tasks may interleave.
#include <atomic>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "movetext.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    for (std::string x; std::getline(ss, x, sep);)
        if (!x.empty()) out.push_back(x);
    return out;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: booklab_rootscore NODES THREADS [EGDB_DIR [HASH_MB]] < tasks.tsv\n";
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
    std::vector<std::vector<std::string>> tasks;
    for (std::string line; std::getline(std::cin, line);) {
        std::vector<std::string> f;
        std::stringstream ss(line);
        for (std::string x; std::getline(ss, x, '\t');) f.push_back(x);
        while (f.size() < 4) f.push_back("");
        if (!f[0].empty()) tasks.push_back(f);
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
            for (std::size_t i; (i = next++) < tasks.size();) {
                const auto& f = tasks[i];
                Position pos;
                std::ostringstream rows;
                if (!f[1].empty() && !pos.set_fen(f[1])) {
                    std::lock_guard l(out_mu);
                    std::cout << f[0] << "\tERR\tFEN\n";
                    continue;
                }
                std::vector<Position> history;
                bool ok = true;
                int ply = 0;
                for (const std::string& t : split(f[2], ' ')) {
                    Move m;
                    if (!parse_move(pos, t, m) || booklab::matching_moves(pos, t) != 1) {
                        rows << f[0] << "\tERR\t" << ply << '\t' << t << '\n';
                        ok = false;
                        break;
                    }
                    history.push_back(pos);
                    pos.make_move(m);
                    ++ply;
                }
                if (ok) {
                    std::vector<std::pair<std::string, std::vector<Move>>> searches{{"root", {}}};
                    for (const std::string& t : split(f[3], ',')) {
                        Move m;
                        if (!parse_move(pos, t, m) || booklab::matching_moves(pos, t) != 1) {
                            rows << f[0] << "\tERR\tscore-move\t" << t << '\n';
                            continue;
                        }
                        searches.push_back({booklab::move_text(pos, m), {m}});
                    }
                    for (const auto& [label, sm] : searches) {
                        engine.new_game();  // cleared hash for every search
                        engine.set_position(pos, history);
                        Limits lim;
                        lim.nodes = nodes;
                        lim.use_book = false;
                        lim.searchmoves = sm;
                        const SearchResult r = engine.go(lim);
                        std::string pv;
                        Position p = pos;
                        for (const Move& m : r.pv) {
                            pv += (pv.empty() ? "" : " ") + booklab::move_text(p, m);
                            p.make_move(m);
                        }
                        rows << f[0] << '\t' << label << '\t' << booklab::move_text(pos, r.best) << '\t'
                             << (r.completed_score == -SCORE_INF ? std::string("none") : std::to_string(r.completed_score))
                             << '\t' << r.depth << '\t' << r.nodes << '\t' << (r.partial ? 1 : 0) << '\t' << pv << '\n';
                    }
                }
                std::lock_guard l(out_mu);
                std::cout << rows.str() << std::flush;
            }
        });
    for (auto& t : pool) t.join();
    return 0;
}
