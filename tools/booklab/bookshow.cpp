// booklab_show: prints a book's entries (move, score, depth) and avoid entries for each FEN read from stdin.
//   booklab_show book.bin < fens.txt
#include <algorithm>
#include <iostream>
#include <string>

#include "book.hpp"
#include "position.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: booklab_show book.bin < fens.txt\n";
        return 2;
    }
    Book book;
    if (!book.load(argv[1])) {
        std::cerr << "cannot load " << argv[1] << '\n';
        return 1;
    }
    std::string fen;
    while (std::getline(std::cin, fen)) {
        if (fen.empty()) continue;
        Position pos;
        if (!pos.set_fen(fen)) {
            std::cout << fen << ": bad FEN\n";
            continue;
        }
        auto es = book.lookup(pos);
        std::sort(es.begin(), es.end(), [](const Book::Entry& a, const Book::Entry& b) { return a.score > b.score; });
        std::cout << fen << ":";
        for (const Book::Entry& e : es)
            if (auto m = Book::to_move(pos, e))
                std::cout << ' ' << move_to_string(*m) << ' ' << e.score << " (d" << int(e.depth) << ")"
                          << (book.avoids(pos, *m) ? " [avoid]" : "") << ';';
        if (es.empty()) std::cout << " (not in book)";
        std::cout << '\n';
    }
    return 0;
}
