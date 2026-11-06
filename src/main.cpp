#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "bench.hpp"
#include "movegen.hpp"
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
                 "  eval                static evaluation (side to move)\n"
                 "  perft <depth>       count leaf nodes\n"
                 "  divide <depth>      perft split by root move\n"
                 "  go [depth N] [movetime MS] [nodes N] [btime MS wtime MS binc MS winc MS movestogo N]\n"
                 "  set threads|hash|<search param> <value>\n"
                 "  bench [depth]       fixed-depth search over built-in positions (node count signature)\n"
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
              << i.time_ms << " hashfull " << i.hashfull << " pv " << pv_string(i.pv) << std::endl;
}

}  // namespace

int main(int argc, char** argv) {
    Engine engine;
    Position pos;
    std::vector<Position> history;

    if (argc > 1 && std::string(argv[1]) == "bench") {
        run_bench(engine, argc > 2 ? std::atoi(argv[2]) : 0);
        return 0;
    }

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
            std::cout << "eval " << evaluate(pos) << std::endl;
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
            if (name != "evalfile") in >> value;
            if (name == "evalfile") {
                std::string path = line.substr(line.find("evalfile") + 9);
                auto ev = path == "hce" ? Evaluator::hce() : Evaluator::from_file(path);
                if (ev) engine.set_evaluator(ev);
                else std::cout << "cannot load " << path << std::endl;
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
