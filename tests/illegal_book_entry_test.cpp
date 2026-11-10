// An illegal book entry cannot win the choice: Book::probe skips entries whose move is not legal in the position, so
// a high-scored bogus entry does not displace the valid one (it used to make probe give up on the position).
#include <iostream>

#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"

using namespace regicide;

int main() {
    const Position start;
    MoveList legal;
    generate_moves(start, legal);
    Book book;
    book.add(Book::make_entry(start, legal[0], 100, 5));
    book.add(Book::make_entry(start, Move{0, 4, 0}, 1000, 5));  // valid squares, but not a legal black move here
    book.finalize();
    const auto chosen = book.probe(start);
    if (!chosen || *chosen != legal[0]) {
        std::cerr << "FAIL: an illegal entry displaced a valid book choice\n";
        return 1;
    }
    return 0;
}
