// The book is not used inside a run of king moves: book entries are keyed by the position alone, but a position
// reached after reversible moves also carries a no-progress count and repetitions, which the book knows nothing
// about. A book move for the root is on offer here, yet the engine has to search.
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

}  // namespace

int main() {
    Position p;
    check(p.set_fen("B:WK28:BK7,K24"), "set_fen");
    std::vector<Position> history;  // the positions before the root
    for (const char* text : {"24-19", "28-32"}) {
        Move m;
        check(parse_move(p, text, m), std::string(text) + " is legal");
        history.push_back(p);
        p.make_move(m);
    }

    MoveList moves;
    generate_moves(p, moves);
    auto book = std::make_shared<Book>();
    book->add(Book::make_entry(p, moves[0], 100, 12));
    book->finalize();

    Engine e;
    e.set_hash_mb(1);
    e.set_book(book);
    e.set_position(p, history);
    Limits l;
    l.depth = 2;
    check(e.go(l).nodes > 0, "a root inside a king-move run is searched, not taken from the book");
    return failures == 0 ? 0 : 1;
}
