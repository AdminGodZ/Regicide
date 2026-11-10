// booklab_reach: can a decision's position still arise from its ballot under a given book?
//
//   booklab_reach BOOK [NODE_LIMIT [EGDB_DIR]] < queries.tsv
// stdin, one query per line:  ID <TAB> ballot moves from the start position <TAB> target FEN <TAB> max plies
// optional 5th field: witness moves after the ballot (checked against the book instead of searching; the ply budget,
// counted from the start position with the ballot, applies when > 0)
// stdout, one line per query: ID <TAB> reachable|unreachable|unknown <TAB> nodes <TAB> path (when reachable)
//
// Search: depth-first from the position after the ballot. "Our" side is the side to move in the target. Where our side
// is to move, the engine would consult its book there (book_applies(): the same test as Engine::go) and the book has
// a move for the position (Book::probe, best move, avoid list applied), only that move is tried; otherwise every legal
// move the engine may choose: the legal moves minus the book's avoid list (all of them if every move is avoided, and
// at database-decided roots, where the engine does not apply the list). The opponent may play any legal move.
// "reachable" therefore still over-approximates (the search may not choose that move); "unreachable" is a proof.
// Pruned by necessary conditions for reaching the target: per colour, men and pieces never increase; men only move
// forward (for every row r, the target cannot have more men of a colour at or behind... see can_reach()); the ply
// budget (max plies from the start position). Visited (position, ply) pairs are not searched twice.
// "unreachable" is a proof within the ply budget; "unknown" means the node limit was hit.
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "book.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "movetext.hpp"
#include "position.hpp"

using namespace regicide;

namespace {

struct Counts {
    int men[2] = {0, 0}, pieces[2] = {0, 0};
    int men_by_row[2][8] = {};  // PDN rows 0..7 (squares 1-4 = row 0)
};

Counts counts(const Position& p) {
    Counts c;
    // read the PDN FEN: robust against internal layout
    const std::string fen = p.fen();
    std::size_t a = fen.find(':');
    while (a != std::string::npos) {
        std::size_t b = fen.find(':', a + 1);
        const std::string part = fen.substr(a + 1, b == std::string::npos ? std::string::npos : b - a - 1);
        if (!part.empty()) {
            const int side = part[0] == 'B' ? 0 : 1;
            std::stringstream ss(part.substr(1));
            for (std::string tok; std::getline(ss, tok, ',');) {
                if (tok.empty()) continue;
                const bool king = tok[0] == 'K';
                const int sq = std::stoi(king ? tok.substr(1) : tok);
                ++c.pieces[side];
                if (!king) ++c.men[side], ++c.men_by_row[side][(sq - 1) / 4];
            }
        }
        a = b;
    }
    return c;
}

// Necessary conditions for P -> T. Black men move to higher rows, white men to lower rows.
bool can_reach(const Counts& p, const Counts& t) {
    for (int s = 0; s < 2; ++s) {
        if (t.men[s] > p.men[s] || t.pieces[s] > p.pieces[s]) return false;
        if (t.pieces[s] - t.men[s] > (p.pieces[s] - p.men[s]) + (p.men[s] - t.men[s])) return false;  // kings come from men
    }
    // black: men at rows <= r in T must come from men at rows <= r in P
    int cp = 0, ct = 0;
    for (int r = 0; r < 8; ++r) {
        cp += p.men_by_row[0][r], ct += t.men_by_row[0][r];
        if (ct > cp) return false;
    }
    cp = ct = 0;
    for (int r = 7; r >= 0; --r) {
        cp += p.men_by_row[1][r], ct += t.men_by_row[1][r];
        if (ct > cp) return false;
    }
    return true;
}

int egdb_pieces = 0;

// The same test as Engine::go (src/search.cpp, master 2efe11a) before its book probe: no book move inside a run of
// king moves (reversible_plies() > 0), nor from a root the endgame database has decided (win or loss).
bool book_applies(const Position& p) {
    if (p.reversible_plies() > 0) return false;
    if (egdb_pieces && p.piece_count() <= egdb_pieces) {
        const egdb::Wld r = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
        if (r == egdb::Wld::Win || r == egdb::Wld::Loss) return false;
    }
    return true;
}

bool db_decided(const Position& p) {
    if (!egdb_pieces || p.piece_count() > egdb_pieces) return false;
    const egdb::Wld r = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
    return r == egdb::Wld::Win || r == egdb::Wld::Loss;
}

// Our moves the engine can play: the book move where it consults the book and has one; otherwise the legal moves
// minus those on the avoid list (Engine::go: unless that leaves none, and not at a database-decided root).
std::vector<Move> own_moves(const Book& book, const Position& p) {
    if (book_applies(p))
        if (auto m = book.probe(p)) return {*m};
    MoveList list;
    generate_moves(p, list);
    std::vector<Move> all(list.begin(), list.end()), kept;
    if (db_decided(p)) return all;
    for (const Move& m : all)
        if (!book.avoids(p, m)) kept.push_back(m);
    return kept.empty() ? all : kept;
}

struct Search {
    const Book& book;
    Key target;
    Counts tc;
    Color ours;
    long limit, nodes = 0;
    bool hit_limit = false;
    std::set<std::pair<Key, int>> seen{};  // (position, 2 * ply + inside a run of king moves)
    std::vector<std::string> path{};

    bool dfs(const Position& p, int ply, int maxply) {
        if (p.key() == target) return true;
        if (ply >= maxply) return false;
        if (++nodes > limit) {
            hit_limit = true;
            return false;
        }
        // the book's eligibility depends on the run of king moves, so it is part of the state
        if (!seen.insert({p.key(), ply * 2 + (p.reversible_plies() > 0)}).second) return false;
        if (!can_reach(counts(p), tc)) return false;
        std::vector<Move> moves;
        if (p.side_to_move() == ours) {
            moves = own_moves(book, p);
        } else {
            MoveList list;
            generate_moves(p, list);
            moves.assign(list.begin(), list.end());
        }
        for (const Move& m : moves) {
            Position c = p;
            c.make_move(m);
            path.push_back(booklab::move_text(p, m));
            if (dfs(c, ply + 1, maxply)) return true;
            path.pop_back();
            if (hit_limit) return false;
        }
        return false;
    }
};

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: booklab_reach BOOK [NODE_LIMIT] < queries.tsv\n";
        return 2;
    }
    Book book;
    if (!book.load(argv[1])) {
        std::cerr << "cannot load " << argv[1] << '\n';
        return 1;
    }
    const long limit = argc > 2 ? std::stol(argv[2]) : 2000000;
    if (argc > 3) {  // endgame database, so that database-decided roots skip the book as in the engine
        if (!egdb::init(argv[3], 512)) {
            std::cerr << "no endgame database in " << argv[3] << '\n';
            return 1;
        }
        egdb_pieces = egdb::max_pieces();
    }
    std::string line;
    while (std::getline(std::cin, line)) {
        std::vector<std::string> f;
        std::stringstream ss(line);
        for (std::string x; std::getline(ss, x, '\t');) f.push_back(x);
        if (f.size() < 4) continue;
        Position start, target;
        bool ok = target.set_fen(f[2]);
        std::stringstream ms(f[1]);
        int ply = 0;
        for (std::string t; ok && ms >> t; ++ply) {
            Move m;
            ok = parse_move(start, t, m);
            if (ok) start.make_move(m);
        }
        if (!ok) {
            std::cout << f[0] << "\terror\t0\t-\n";
            continue;
        }
        Search s{book, target.key(), counts(target), target.side_to_move(), limit};
        if (f.size() >= 5 && !f[4].empty()) {
            // witness mode: the given moves (after the ballot) must reach the target, and wherever our side is to move
            // and the book has a move, the witness must play exactly that move
            Position p = start;
            std::stringstream ws(f[4]);
            std::string why;
            const int maxply = std::stoi(f[3]);  // 0: no limit
            int wply = ply;                      // plies from the start position, the ballot included
            for (std::string t; ws >> t;) {
                if (p.key() == target.key()) break;
                if (maxply > 0 && wply >= maxply) { why = "witness longer than the ply budget"; break; }
                Move m;
                if (!parse_move(p, t, m)) { why = "illegal " + t; break; }
                if (p.side_to_move() == s.ours) {
                    const std::vector<Move> ok_moves = own_moves(book, p);
                    if (std::find(ok_moves.begin(), ok_moves.end(), m) == ok_moves.end()) {
                        why = book_applies(p) && book.probe(p) ? "book plays " + booklab::move_text(p, ok_moves[0]) + " not " + t
                                                               : "avoid list forbids " + t;
                        break;
                    }
                }
                p.make_move(m);
                ++wply;
            }
            const bool hit = why.empty() && p.key() == target.key();
            std::cout << f[0] << '\t' << (hit ? "reachable" : "witness-failed") << "\t0\t" << (hit ? f[4] : why) << '\n';
            continue;
        }
        const bool r = s.dfs(start, ply, std::stoi(f[3]));
        std::string p;
        for (const auto& m : s.path) p += (p.empty() ? "" : " ") + m;
        std::cout << f[0] << '\t' << (r ? "reachable" : s.hit_limit ? "unknown" : "unreachable") << '\t' << s.nodes
                  << '\t' << (r ? p : "-") << '\n';
    }
    return 0;
}
