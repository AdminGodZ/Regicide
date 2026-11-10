// booklab_replay: replays games with the engine's own rules (Position, parse_move) for the theory table.
//
// stdin, one game per line:  GID <TAB> FEN (empty = start position) <TAB> moves (PDN notation, space separated)
// stdout:
//   P <TAB> GID <TAB> ply <TAB> key (hex) <TAB> move    for every move played (ply 0 = the game's first position)
//   F <TAB> key <TAB> FEN                               the first time a position is seen
//   E <TAB> GID <TAB> ply <TAB> token                   illegal or unparsable move (the rest of that game is skipped)
//   A <TAB> GID <TAB> ply <TAB> token                   token matches several capture routes (first one used)
//   Z <TAB> GID <TAB> plies <TAB> FEN                   the position after the last move (end of each game)
// Moves are printed with booklab::move_text (captures with their full jump path).
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>

#include "movegen.hpp"
#include "position.hpp"
#include "movetext.hpp"

using namespace regicide;

int main() {
    std::ios::sync_with_stdio(false);
    std::unordered_set<Key> seen;
    std::string line;
    char hex[17];
    while (std::getline(std::cin, line)) {
        const auto t1 = line.find('\t');
        if (t1 == std::string::npos) continue;
        const auto t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) continue;
        const std::string gid = line.substr(0, t1), fen = line.substr(t1 + 1, t2 - t1 - 1);
        Position pos;
        if (!fen.empty() && !pos.set_fen(fen)) {
            std::cout << "E\t" << gid << "\t0\tFEN:" << fen << '\n';
            continue;
        }
        std::istringstream in(line.substr(t2 + 1));
        std::string tok;
        int ply = 0;
        while (in >> tok) {
            Move m;
            if (!parse_move(pos, tok, m)) {
                std::cout << "E\t" << gid << '\t' << ply << '\t' << tok << '\n';
                break;
            }
            if (booklab::matching_moves(pos, tok) > 1)  // the game record itself does not say which capture route
                std::cout << "A\t" << gid << '\t' << ply << '\t' << tok << '\n';
            std::snprintf(hex, sizeof hex, "%016llx", (unsigned long long)pos.key());
            if (seen.insert(pos.key()).second) std::cout << "F\t" << hex << '\t' << pos.fen() << '\n';
            std::cout << "P\t" << gid << '\t' << ply << '\t' << hex << '\t' << booklab::move_text(pos, m) << '\n';
            pos.make_move(m);
            ++ply;
        }
        std::cout << "Z\t" << gid << '\t' << ply << '\t' << pos.fen() << '\n';  // the position after the last move
    }
    return 0;
}
