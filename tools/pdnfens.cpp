// Replays PDN games (stdin) and prints every position: game, ply, side to move, pieces, FEN and the game's tags.
//   pdnfens < games.pdn > positions.tsv
// Columns: game ply stm pieces fen result black white round legal-moves. A move that does not parse ends that game (reported).
#include <iostream>
#include <regex>
#include <sstream>
#include <string>

#include "movegen.hpp"
#include "position.hpp"

using namespace regicide;

int main() {
    std::stringstream ss;
    ss << std::cin.rdbuf();
    const std::string text = ss.str();
    std::size_t at = 0;
    int game = 0;
    while ((at = text.find("[Event", at)) != std::string::npos) {
        std::size_t next = text.find("\n[Event", at + 1);
        const std::string g = text.substr(at, next == std::string::npos ? std::string::npos : next + 1 - at);
        at = next == std::string::npos ? text.size() : next + 1;
        ++game;
        auto tag = [&](const std::string& name) {
            std::smatch m;
            std::regex re("\\[" + name + " \"([^\"]*)\"\\]");
            return std::regex_search(g, m, re) ? m[1].str() : std::string();
        };
        Position pos;
        const std::string fen = tag("FEN");
        if (!fen.empty() && !pos.set_fen(fen)) {
            std::cerr << "game " << game << ": bad FEN\n";
            continue;
        }
        const std::string result = tag("Result"), black = tag("Black"), white = tag("White"), round = tag("Round");
        std::string body = g.substr(g.rfind(']') + 1);
        body = std::regex_replace(body, std::regex("\\{[^}]*\\}"), " ");
        // Result tokens look like moves ("1-0", the "2-1" inside "1/2-1/2"): remove them first.
        body = std::regex_replace(body, std::regex("(^|\\s)(1-0|0-1|1/2-1/2|2-0|0-2|1-1|\\*)(?=\\s|$)"), " ");
        std::regex mv("\\b\\d+(?:[-x]\\d+)+\\b");
        int ply = 0;
        auto emit = [&] {
            MoveList legal;
            generate_moves(pos, legal);
            std::cout << game << '\t' << ply << '\t' << (pos.side_to_move() == BLACK ? 'B' : 'W') << '\t'
                      << pos.piece_count() << '\t' << pos.fen() << '\t' << result << '\t' << black << '\t' << white
                      << '\t' << round << '\t' << legal.size() << '\n';
        };
        emit();
        for (auto it = std::sregex_iterator(body.begin(), body.end(), mv); it != std::sregex_iterator(); ++it) {
            Move m;
            if (!parse_move(pos, it->str(), m)) {
                std::cerr << "game " << game << " round " << round << ": cannot parse " << it->str() << " at ply "
                          << ply << '\n';
                break;
            }
            pos.make_move(m);
            ++ply;
            emit();
        }
    }
    return 0;
}
