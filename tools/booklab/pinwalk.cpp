// booklab_pinwalk (WP11): positions with our side to move along played games, for pinning validated lines.
//
//   booklab_pinwalk BOOK [EGDB_DIR] < games.tsv
// stdin, one game per line: GID <TAB> FEN (empty = start) <TAB> our colour (B/W) <TAB> first ply to report <TAB> moves
// stdout, one line per position with our side to move, from the first ply until the position has <= 8 pieces or
// the game ends:
//   GID <TAB> ply <TAB> FEN <TAB> move played (full capture path) <TAB> eligible (1/0) <TAB> book move or "-"
//   <TAB> moves on the book's avoid list in this position (comma-separated) or "-" (review 2026-11-11: a pin must
//   never be an avoided move, and a later avoided own move makes a lost game unplayable just as a forced move does)
// eligible: the engine would consult its book in this position (Engine::go: reversible_plies() == 0 and the root not
// decided by the endgame database), as in booklab_reach.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "book.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "movetext.hpp"
#include "position.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: booklab_pinwalk BOOK [EGDB_DIR] < games.tsv\n";
        return 2;
    }
    Book book;
    if (!book.load(argv[1])) {
        std::cerr << "cannot load " << argv[1] << '\n';
        return 1;
    }
    int egdb_pieces = 0;
    if (argc > 2) {
        if (!egdb::init(argv[2], 512)) {
            std::cerr << "no endgame database in " << argv[2] << '\n';
            return 1;
        }
        egdb_pieces = egdb::max_pieces();
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        std::vector<std::string> f;
        std::stringstream ss(line);
        for (std::string x; std::getline(ss, x, '\t');) f.push_back(x);
        if (f.size() < 5) continue;
        Position pos;
        if (!f[1].empty() && !pos.set_fen(f[1])) {
            std::cout << f[0] << "\tERR\tFEN\n";
            continue;
        }
        const Color ours = f[2] == "B" ? BLACK : WHITE;
        const int first = std::stoi(f[3]);
        // The whole record is replayed before anything is printed: a game with an illegal or ambiguous move anywhere
        // (also after the 8-piece point) yields only its ERR line, never a prefix.
        std::stringstream ms(f[4]);
        std::ostringstream rows;
        std::string err;
        bool reporting = true;
        int ply = 0;
        for (std::string t; ms >> t; ++ply) {
            Move m;
            if (!parse_move(pos, t, m) || booklab::matching_moves(pos, t) != 1) {
                err = std::to_string(ply) + '\t' + t;
                break;
            }
            if (pos.piece_count() <= 8) reporting = false;
            if (reporting && ply >= first && pos.side_to_move() == ours) {
                bool eligible = pos.reversible_plies() == 0;
                if (eligible && egdb_pieces && pos.piece_count() <= egdb_pieces) {
                    const egdb::Wld r = egdb::probe(pos.pieces(BLACK), pos.pieces(WHITE), pos.kings(), pos.side_to_move());
                    eligible = !(r == egdb::Wld::Win || r == egdb::Wld::Loss);
                }
                const auto bm = book.probe(pos);
                std::string avoided;
                MoveList legal;
                generate_moves(pos, legal);
                for (const Move& lm : legal)
                    if (book.avoids(pos, lm)) avoided += (avoided.empty() ? "" : ",") + booklab::move_text(pos, lm);
                rows << f[0] << '\t' << ply << '\t' << pos.fen() << '\t' << booklab::move_text(pos, m) << '\t'
                     << (eligible ? 1 : 0) << '\t' << (bm ? booklab::move_text(pos, *bm) : "-") << '\t'
                     << (avoided.empty() ? "-" : avoided) << '\n';
            }
            pos.make_move(m);
        }
        if (!err.empty()) std::cout << f[0] << "\tERR\t" << err << '\n';
        else std::cout << rows.str();
    }
    return 0;
}
