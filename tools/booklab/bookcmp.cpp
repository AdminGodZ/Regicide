// booklab_bookcmp: compares what two books play (Book::probe, best move, each book's own avoid list) in a list of
// positions.
//   booklab_bookcmp A.bin B.bin < fens.txt
// stdout: one line per position where the two books differ: FEN <TAB> move of A (or "-") <TAB> move of B (or "-");
// last line: "compared N positions, D differ".
#include <iostream>
#include <string>

#include "book.hpp"
#include "movetext.hpp"
#include "position.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: booklab_bookcmp A.bin B.bin < fens.txt\n";
        return 2;
    }
    Book a, b;
    if (!a.load(argv[1]) || !b.load(argv[2])) {
        std::cerr << "cannot load the books\n";
        return 1;
    }
    std::string fen;
    long n = 0, d = 0;
    while (std::getline(std::cin, fen)) {
        if (fen.empty()) continue;
        Position p;
        if (!p.set_fen(fen)) {
            std::cerr << "bad FEN " << fen << '\n';
            return 1;
        }
        ++n;
        const auto ma = a.probe(p), mb = b.probe(p);
        const bool same = (!ma && !mb) || (ma && mb && *ma == *mb);
        if (!same) {
            ++d;
            std::cout << fen << '\t' << (ma ? booklab::move_text(p, *ma) : "-") << '\t'
                      << (mb ? booklab::move_text(p, *mb) : "-") << '\n';
        }
    }
    std::cout << "compared " << n << " positions, " << d << " differ\n";
    return 0;
}
