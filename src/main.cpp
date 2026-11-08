#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bench.hpp"
#include "datafile.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "nnue.hpp"
#include "perft.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

void print_help() {
    std::cout << "Commands:\n"
                 "  position startpos|fen <FEN> [moves <m1> <m2> ...]\n"
                 "  newgame             clear hash and history tables\n"
                 "  moves               list legal moves\n"
                 "  play <move|index>   play a move (PDN like 11-15 / 22x15, or an index from 'moves')\n"
                 "  d                   print the board\n"
                 "  eval                static evaluation (side to move) with the current evaluator\n"
                 "  evalpos <FEN>       static evaluation of a position\n"
                 "  perft <depth>       count leaf nodes\n"
                 "  divide <depth>      perft split by root move\n"
                 "  go [depth N] [movetime MS] [nodes N] [btime MS wtime MS binc MS winc MS movestogo N]\n"
                 "  set threads|hash|<search param> <value>\n"
                 "  set evalfile <path|builtin|hce>  linear weights (RGEV) or an NNUE net (RGNN)\n"
                 "  set book <path|builtin|off>, set egdb <dir>\n"
                 "  set avoidfile <path|builtin>  avoid list (moves never played) from a book file; set avoid 0|1\n"
                 "  bench [depth]       fixed-depth search over built-in positions (node count signature)\n"
                 "Command line: regicide bench [depth] | regicide evaldata <evalfile|builtin> <data.bin> [count]\n"
                 "              regicide nnuebench <evalfile|builtin> [data.bin] [count]  NNUE stage timings\n"
                 "  quit\n";
}

std::string pv_string(const std::vector<Move>& pv) {
    std::string s;
    for (const Move& m : pv) s += (s.empty() ? "" : " ") + move_to_string(m);
    return s;
}

void print_info(const SearchInfo& i) {
    std::cout << "info depth " << i.depth << " seldepth " << i.seldepth << " score " << i.score << " nodes "
              << i.nodes << " nps " << i.nodes * 1000 / std::max<std::int64_t>(1, i.time_ms) << " time "
               << i.time_ms << " hashfull " << i.hashfull << " tbhits " << i.tbhits << " pv " << pv_string(i.pv) << std::endl;
}

// Prints the static eval of each training record (side to move's view), one per line: for checking a net export.
int eval_data(const std::string& evalfile, const std::string& data, long count) {
    std::string error;
    const auto ev = evalfile == "builtin" ? Evaluator::builtin() : Evaluator::from_file(evalfile, &error);
    if (!ev) {
        std::cerr << "cannot load " << evalfile << ": " << error << std::endl;
        return 1;
    }
    std::ifstream in(data, std::ios::binary);
    if (!in) {
        std::cerr << "cannot open " << data << std::endl;
        return 1;
    }
    DataRecord r;
    for (long n = 0; (count <= 0 || n < count) && in.read(reinterpret_cast<char*>(&r), sizeof r); ++n)
        std::cout << (*ev)(r.position()) << '\n';
    return 0;
}

// Times the NNUE stages (Network::profile) on the first `count` training records, or on the bench positions.
int nnue_bench(const std::string& evalfile, const std::string& data, long count) {
    std::string error;
    const auto ev = evalfile == "builtin" ? Evaluator::builtin() : Evaluator::from_file(evalfile, &error);
    if (!ev || !ev->network()) {
        std::cerr << "cannot load a net from " << evalfile << (error.empty() ? "" : ": " + error) << std::endl;
        return 1;
    }
    std::vector<Position> positions;
    if (data.empty()) {
        positions = bench_positions();
    } else {
        std::ifstream in(data, std::ios::binary);
        DataRecord r;
        while ((count <= 0 || long(positions.size()) < count) && in.read(reinterpret_cast<char*>(&r), sizeof r))
            positions.push_back(r.position());
    }
    ev->network()->profile(positions, std::cout);
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    Engine engine;
    engine.set_book(Book::builtin());
    std::shared_ptr<const Book> avoid_list = Book::builtin();
    bool avoid_on = true;
    engine.set_avoid(avoid_list);
    auto load_egdb = [&](const std::string& path) {
        if (egdb::init(path, 4096)) {
            engine.set_egdb_pieces(egdb::max_pieces());
            std::cout << "egdb: " << egdb::max_pieces() << " pieces (mtc " << egdb::max_mtc_pieces() << ") from "
                      << path << std::endl;
        } else {
            std::cout << "egdb: nothing found in " << path << std::endl;
        }
    };
    if (const char* env = std::getenv("REGICIDE_EGDB")) load_egdb(env);
    Position pos;
    std::vector<Position> history;

    if (argc > 1 && std::string(argv[1]) == "bench") {
        run_bench(engine, argc > 2 ? std::atoi(argv[2]) : 0);
        return 0;
    }
    if (argc > 3 && std::string(argv[1]) == "evaldata")
        return eval_data(argv[2], argv[3], argc > 4 ? std::atol(argv[4]) : 0);
    if (argc > 2 && std::string(argv[1]) == "nnuebench")
        return nnue_bench(argv[2], argc > 3 ? argv[3] : "", argc > 4 ? std::atol(argv[4]) : 1024);

    std::cout << "Regicide 0.2 (type 'help')" << std::endl;

    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;

        if (cmd.empty()) continue;
        if (cmd == "quit" || cmd == "exit") break;

        if (cmd == "help") {
            print_help();
        } else if (cmd == "position") {
            std::string kind, tok;
            in >> kind;
            Position p;
            if (kind == "fen") {
                in >> tok;
                if (!p.set_fen(tok)) {
                    std::cout << "invalid fen" << std::endl;
                    continue;
                }
            } else if (kind != "startpos") {
                std::cout << "usage: position startpos|fen <FEN> [moves ...]" << std::endl;
                continue;
            }
            history.clear();
            if (in >> tok && tok == "moves") {
                while (in >> tok) {
                    Move m;
                    if (!parse_move(p, tok, m)) {
                        std::cout << "illegal move " << tok << std::endl;
                        break;
                    }
                    history.push_back(p);
                    p.make_move(m);
                }
            }
            pos = p;
        } else if (cmd == "newgame") {
            engine.new_game();
        } else if (cmd == "d") {
            std::cout << pos.pretty() << pos.fen() << std::endl;
        } else if (cmd == "eval") {
            std::cout << "eval " << engine.evaluator()(pos) << std::endl;
        } else if (cmd == "evalpos") {
            std::string fen;
            in >> fen;
            Position p;
            if (p.set_fen(fen)) std::cout << "eval " << engine.evaluator()(p) << std::endl;
            else std::cout << "invalid fen" << std::endl;
        } else if (cmd == "moves") {
            MoveList list;
            generate_moves(pos, list);
            for (std::size_t i = 0; i < list.size(); ++i) std::cout << i << ": " << move_to_string(list[i]) << '\n';
            std::cout << std::flush;
        } else if (cmd == "play") {
            std::string tok;
            in >> tok;
            MoveList list;
            generate_moves(pos, list);
            Move m;
            bool ok = parse_move(pos, tok, m);
            if (!ok && !tok.empty() && std::all_of(tok.begin(), tok.end(), ::isdigit)) {
                std::size_t idx = std::stoul(tok);
                if ((ok = idx < list.size())) m = list[idx];
            }
            if (ok) {
                history.push_back(pos);
                pos.make_move(m);
            } else {
                std::cout << "illegal move" << std::endl;
            }
        } else if (cmd == "perft" || cmd == "divide") {
            int depth = 1;
            in >> depth;
            auto start = std::chrono::steady_clock::now();
            std::uint64_t nodes = cmd == "perft" ? perft(pos, depth) : perft_divide(pos, depth);
            double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            std::cout << "perft(" << depth << ") = " << nodes << "  (" << secs << " s, "
                      << static_cast<std::uint64_t>(nodes / std::max(secs, 1e-9)) << " nps)" << std::endl;
        } else if (cmd == "go") {
            Limits limits;
            std::string kw;
            int btime = 0, wtime = 0, binc = 0, winc = 0;
            while (in >> kw) {
                long long v = 0;
                if (kw == "infinite") {
                    limits.infinite = true;
                    continue;
                }
                in >> v;
                if (kw == "depth") limits.depth = std::clamp(int(v), 1, MAX_PLY - 1);
                else if (kw == "movetime") limits.movetime_ms = int(v);
                else if (kw == "nodes") limits.nodes = std::uint64_t(v);
                else if (kw == "btime") btime = int(v);
                else if (kw == "wtime") wtime = int(v);
                else if (kw == "binc") binc = int(v);
                else if (kw == "winc") winc = int(v);
                else if (kw == "movestogo") limits.moves_to_go = int(v);
            }
            const bool black = pos.side_to_move() == BLACK;
            limits.time_ms = black ? btime : wtime;
            limits.inc_ms = black ? binc : winc;
            engine.set_position(pos, history);
            SearchResult r = engine.go(limits, print_info);
            if (r.best.is_null() && !r.best.is_capture())
                std::cout << "bestmove (none)" << std::endl;
            else
                std::cout << "bestmove " << move_to_string(r.best) << std::endl;
        } else if (cmd == "set") {
            std::string name;
            long long value = 0;
            in >> name;
            if (name != "evalfile" && name != "book" && name != "egdb" && name != "avoidfile") in >> value;
            if (name == "egdb") {
                load_egdb(line.substr(line.find("egdb") + 5));
                continue;
            }
            if (name == "book") {
                std::string path = line.substr(line.find("book") + 5);
                if (path == "off") engine.set_book(nullptr);
                else if (path == "builtin") engine.set_book(Book::builtin());
                else if (auto b = std::make_shared<Book>(); b->load(path)) engine.set_book(b);
                else std::cout << "cannot load " << path << std::endl;
                continue;
            }
            if (name == "avoidfile" || name == "avoid") {
                if (name == "avoid") {
                    avoid_on = value != 0;
                } else {
                    std::string path = line.substr(line.find("avoidfile") + 10);
                    auto b = std::make_shared<Book>();
                    if (path == "builtin") avoid_list = Book::builtin();
                    else if (b->load(path)) avoid_list = b;
                    else std::cout << "cannot load " << path << std::endl;
                }
                engine.set_avoid(avoid_on ? avoid_list : nullptr);
                std::cout << "avoid: " << (avoid_on ? avoid_list->avoid_size() : 0) << " entries" << std::endl;
                continue;
            }
            if (name == "evalfile") {
                std::string path = line.substr(line.find("evalfile") + 9), error;
                auto ev = path == "hce" ? Evaluator::hce()
                          : path == "builtin" ? Evaluator::builtin()
                                              : Evaluator::from_file(path, &error);
                if (!ev) {
                    std::cout << "cannot load " << path << ": " << error << std::endl;
                    continue;
                }
                engine.set_evaluator(ev);
                std::cout << "eval: " << (ev->network() ? "nnue" : ev->trained() ? "linear" : "hce") << std::endl;
                continue;
            }
            if (name == "threads") engine.set_threads(int(value));
            else if (name == "hash") engine.set_hash_mb(std::size_t(value));
            else if (!engine.params.set(name, int(value))) std::cout << "unknown option " << name << std::endl;
        } else if (cmd == "bench") {
            int depth = 0;
            in >> depth;
            run_bench(engine, depth);
        } else {
            std::cout << "unknown command: " << cmd << " (try 'help')" << std::endl;
        }
    }
    return 0;
}
