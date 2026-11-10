// A root the endgame database decides is played from the database, past the book and the avoid list: a book move
// that throws the win is not taken, and the avoid list does not push the engine off the move it would choose.
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

    test_egdb::remove(dir);
    return failures == 0 ? 0 : 1;
}
