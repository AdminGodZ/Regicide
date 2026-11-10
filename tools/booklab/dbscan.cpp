// booklab_dbscan (WP8): database audit of played games.
//
//   booklab_dbscan EGDB_DIR [CACHE_MB] < games.tsv > scan.tsv
// stdin, one game per line: GID <TAB> FEN (empty = start) <TAB> moves (PDN notation)
// stdout, one line per game:
//   GID <TAB> first_win_ply <TAB> winner (B/W) <TAB> pieces <TAB> fen <TAB> throw_ply <TAB> throw_move <TAB> throw_fen
//       <TAB> throws <TAB> final_value <TAB> final_pieces <TAB> plies <TAB> max_quiet
// first_win_ply: first position (ply index, 0 = the game's first position) with at most max_pieces() pieces where the
// database says the side to move wins or loses (then the side that just moved wins); -1 if none. winner: that side.
// throw_ply: first later ply where the winner, to move in a won position, played a move after which the position is
// no longer lost for the opponent (-1 if never); throws: how many such moves. final_value: database value of the last
// position from Black's view (B = Black wins, W = White wins, D = draw, - = more than max_pieces pieces).
// max_quiet: the longest run of plies without a capture or man move (the 40-move rule draws at 79).
// A game with a move that does not parse to exactly one legal move gets the row "GID <TAB> ERR <TAB> ply <TAB> token"
// instead (rejected, never scanned as a prefix).
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "movetext.hpp"
#include "position.hpp"

using namespace regicide;

namespace {

int max_db = 0;

// Database value from Black's view: 'B', 'W', 'D', or '-' (not in the database).
char value(const Position& p) {
    if (p.piece_count() > max_db) return '-';
    const egdb::Wld r = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
    if (r == egdb::Wld::Unknown) return '-';
    if (r == egdb::Wld::Draw) return 'D';
    const bool stm_wins = r == egdb::Wld::Win;
    const bool black_to_move = p.side_to_move() == BLACK;
    return (stm_wins == black_to_move) ? 'B' : 'W';
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: booklab_dbscan EGDB_DIR [CACHE_MB] < games.tsv\n";
        return 2;
    }
    if (!egdb::init(argv[1], argc > 2 ? std::stoul(argv[2]) : 2048)) {
        std::cerr << "no endgame database in " << argv[1] << '\n';
        return 1;
    }
    max_db = egdb::max_pieces();
    std::ios::sync_with_stdio(false);
    std::string line;
    while (std::getline(std::cin, line)) {
        const auto t1 = line.find('\t'), t2 = line.find('\t', t1 + 1);
        if (t1 == std::string::npos || t2 == std::string::npos) continue;
        const std::string gid = line.substr(0, t1), fen = line.substr(t1 + 1, t2 - t1 - 1);
        Position pos;
        if (!fen.empty() && !pos.set_fen(fen)) continue;
        std::vector<std::string> toks;
        {
            std::istringstream in(line.substr(t2 + 1));
            for (std::string t; in >> t;) toks.push_back(t);
        }
        int ply = 0, first = -1, pieces = 0, throw_ply = -1, throws = 0, quiet = 0, max_quiet = 0;
        char winner = 0;
        std::string first_fen, throw_move, throw_fen, bad;
        char v = value(pos);
        for (std::size_t i = 0;; ++i) {
            if (first < 0 && (v == 'B' || v == 'W')) {
                first = ply, winner = v, pieces = pos.piece_count(), first_fen = pos.fen();
            }
            if (i >= toks.size()) break;
            Move m;
            if (!parse_move(pos, toks[i], m) || booklab::matching_moves(pos, toks[i]) != 1) {
                bad = toks[i];  // unparsable, illegal or ambiguous: the game is rejected, not scanned as a prefix
                break;
            }
            const bool winner_to_move = winner && v == winner &&
                                        ((winner == 'B') == (pos.side_to_move() == BLACK));
            // no-progress count: plies since the last capture or man move (the 40-move rule draws at 79 plies)
            if (m.is_capture() || !(pos.kings() & bit(m.from))) quiet = 0;
            else max_quiet = std::max(max_quiet, ++quiet);
            Position next = pos;
            next.make_move(m);
            const char nv = value(next);
            if (winner_to_move && nv != winner) {
                if (throw_ply < 0) throw_ply = ply, throw_move = booklab::move_text(pos, m), throw_fen = pos.fen();
                ++throws;
            }
            pos = next;
            v = nv;
            ++ply;
        }
        if (!bad.empty()) {
            std::cout << gid << "\tERR\t" << ply << '\t' << bad << '\n';
            continue;
        }
        std::cout << gid << '\t' << first << '\t' << (winner ? std::string(1, winner) : "-") << '\t' << pieces << '\t'
                  << (first_fen.empty() ? "-" : first_fen) << '\t' << throw_ply << '\t'
                  << (throw_move.empty() ? "-" : throw_move) << '\t' << (throw_fen.empty() ? "-" : throw_fen) << '\t'
                  << throws << '\t' << v << '\t' << pos.piece_count() << '\t' << ply << '\t' << max_quiet << '\n';
    }
    return 0;
}
