// A root the endgame database decides is played from the database, past the book and the avoid list: a book move
// that throws the win is not taken, and the avoid list does not push the engine off the move it would choose.
// The same for a root the database knows as a draw: neither a book move nor the avoid list makes it a loss.
// Builds a 2..3-piece database (WLD and MTC) in a temporary directory; the engine uses only its WLD values.
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "book.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "test_egdb.hpp"

using namespace regicide;
using egdb::Wld;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

Wld probe(const Position& p) { return egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move()); }

}  // namespace

int main() {
    const std::string dir = test_egdb::build("egdb-root-test", 3);
    check(!dir.empty(), "build and load 2..3 pieces");

    // Black wins; some moves keep the win, at least one throws it.
    Position p;
    check(p.set_fen("B:WK1:BK6,K10"), "set_fen");
    check(probe(p) == Wld::Win, "the root is a database win");
    MoveList moves;
    generate_moves(p, moves);
    std::vector<Move> keep, the_rest;
    for (const Move& m : moves) {
        Position q = p;
        q.make_move(m);
        (probe(q) == Wld::Loss ? keep : the_rest).push_back(m);
    }
    check(keep.size() > 1 && !the_rest.empty(), "several moves keep the win and one does not");

    if (failures == 0) {
        Engine e;
        e.set_hash_mb(1);
        e.set_evaluator(Evaluator::hce());
        e.set_egdb_pieces(3);
        e.set_egdb_mtc_pieces(0);
        Limits l;
        l.depth = 2;

        auto book = std::make_shared<Book>();
        book->add(Book::make_entry(p, the_rest[0], 100, 12));
        book->finalize();
        e.set_book(book);
        e.set_position(p);
        const SearchResult r = e.go(l);
        Position q = p;
        q.make_move(r.best);
        check(probe(q) == Wld::Loss && r.nodes > 0, "a database win is searched, not thrown by the book move");

        e.set_book(nullptr);
        e.new_game();
        e.set_position(p);
        const Move chosen = e.go(l).best;
        auto avoid = std::make_shared<Book>();
        avoid->add_avoid(Book::make_entry(p, chosen, 0, 1));
        avoid->finalize();
        e.set_avoid(avoid);
        e.new_game();
        e.set_position(p);
        check(e.go(l).best == chosen, "a database-decided root ignores the avoid list");
    }

    // A drawn root with a move that loses at once (10-14 is captured, and Black has no piece left).
    Position d;
    check(d.set_fen("B:WK18:BK10"), "set_fen");
    check(probe(d) == Wld::Draw, "the second root is a database draw");
    std::vector<Move> drawing, losing;
    moves.clear();
    generate_moves(d, moves);
    for (const Move& m : moves) {
        Position q = d;
        q.make_move(m);
        (probe(q) == Wld::Draw ? drawing : losing).push_back(m);
    }
    check(!drawing.empty() && !losing.empty(), "some moves keep the draw and one does not");
    if (failures == 0) {
        Engine e;
        e.set_hash_mb(1);
        e.set_evaluator(Evaluator::hce());
        e.set_egdb_pieces(3);
        e.set_egdb_mtc_pieces(0);
        Limits l;
        l.depth = 5;
        auto keeps = [&](const char* what) {
            e.new_game();
            e.set_position(d);
            Position q = d;
            q.make_move(e.go(l).best);
            check(probe(q) == Wld::Draw, what);
        };

        auto book = std::make_shared<Book>();
        book->add(Book::make_entry(d, losing[0], 0, 20));
        book->finalize();
        e.set_book(book);
        keeps("a database draw is not lost by a book move");

        e.set_book(nullptr);
        auto avoid = std::make_shared<Book>();
        for (const Move& m : drawing) avoid->add_avoid(Book::make_entry(d, m, 0, 20));
        avoid->finalize();
        e.set_avoid(avoid);
        keeps("a database draw is not lost by an avoid list that names every drawing move");
    }

    // The tables know the board, not the game: in a lost ending the move that repeats a position for the third
    // time draws at once and is played instead of the longest defence. Every Black move before it is the engine's
    // own table move.
    {
        Position g;
        check(g.set_fen("B:WK21,K25:BK4"), "set_fen");
        std::vector<Position> history;
        bool legal = true;
        for (const char* text : {"4-8", "25-30", "8-11", "21-17", "11-15", "17-13", "15-19", "13-17", "19-15", "17-21",
                                 "15-10", "21-17"}) {
            Move m;
            legal = legal && parse_move(g, text, m);
            history.push_back(g);
            g.make_move(m);
        }
        check(legal && g.fen() == "B:WK17,K30:BK10" && probe(g) == Wld::Loss, "the lost position is reached");
        Move repeat;
        check(parse_move(g, "10-15", repeat), "10-15 is legal");
        Engine e;
        e.set_hash_mb(1);
        e.set_evaluator(Evaluator::hce());
        e.set_egdb_pieces(3);
        e.set_egdb_mtc_pieces(3);
        e.set_position(g, history);
        Limits l;
        l.depth = 4;
        const SearchResult r = e.go(l);
        check(r.best == repeat && r.score == SCORE_DRAW, "a lost root takes the draw by a third occurrence");
        e.new_game();
        e.set_position(g);  // the same board without its history: the table move
        check(e.go(l).best != repeat, "without the history the longest defence is played");
    }

    test_egdb::remove(dir);
    return failures == 0 ? 0 : 1;
}
