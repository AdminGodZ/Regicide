#include <algorithm>
#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include "movegen.hpp"
#include "perft.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

void print_help() {
    std::cout << "Commands:\n"
                 "  position startpos | position fen <FEN>\n"
                 "  moves               list legal moves\n"
                 "  play <index>        play the move at <index> from 'moves'\n"
                 "  d                   print the board\n"
                 "  perft <depth>       count leaf nodes\n"
                 "  divide <depth>      perft split by root move\n"
                 "  go depth <n>        search to depth n\n"
                 "  quit\n";
}

}  // namespace

int main() {
    Position pos;
    std::cout << "Regicide 0.1 (type 'help')\n";

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
            std::string kind;
            in >> kind;
            if (kind == "startpos") {
                pos.set_fen(Position::START_FEN);
            } else if (kind == "fen") {
                std::string fen;
                in >> fen;
                if (!pos.set_fen(fen)) std::cout << "invalid fen\n";
            }
        } else if (cmd == "d") {
            std::cout << pos.pretty() << pos.fen() << '\n';
        } else if (cmd == "moves") {
            MoveList list;
            generate_moves(pos, list);
            for (std::size_t i = 0; i < list.size(); ++i) std::cout << i << ": " << move_to_string(list[i]) << '\n';
        } else if (cmd == "play") {
            std::size_t idx = 0;
            MoveList list;
            generate_moves(pos, list);
            if (in >> idx && idx < list.size())
                pos.make_move(list[idx]);
            else
                std::cout << "invalid move index\n";
        } else if (cmd == "perft" || cmd == "divide") {
            int depth = 1;
            in >> depth;
            auto start = std::chrono::steady_clock::now();
            std::uint64_t nodes = cmd == "perft" ? perft(pos, depth) : perft_divide(pos, depth);
            double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            std::cout << "perft(" << depth << ") = " << nodes << "  (" << secs << " s, "
                      << static_cast<std::uint64_t>(nodes / std::max(secs, 1e-9)) << " nps)\n";
        } else if (cmd == "go") {
            std::string kw;
            int depth = 10;
            in >> kw >> depth;
            SearchResult r = search(pos, depth);
            std::cout << "bestmove " << move_to_string(r.best) << '\n';
        } else {
            std::cout << "unknown command: " << cmd << " (try 'help')\n";
        }
    }
    return 0;
}
