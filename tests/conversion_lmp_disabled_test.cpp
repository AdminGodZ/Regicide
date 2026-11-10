// No late-move pruning in the conversion search: there LMP could skip the only move that keeps a database win and
// store a wrong bound. Switching LMP on (lmp_depth 99) or off (0) must not change the score, the best move or the
// node count of a depth-4 conversion search, for three decided positions and their colour-reflected twins. Builds a
// 2..4-piece database in a temporary directory; the engine uses its WLD values only.
#include <iostream>
#include <string>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "test_egdb.hpp"

using namespace regicide;

namespace {

// The board turned 180 degrees: square i becomes 31 - i.
Bitboard flip(Bitboard b) {
    Bitboard r = 0;
    for (int i = 0; i < 32; ++i)
        if (b & bit(i)) r |= bit(31 - i);
    return r;
}

SearchResult search(const Position& p, int lmp_depth) {
    Engine engine;
    engine.set_hash_mb(1);
    engine.set_evaluator(Evaluator::hce());
    engine.set_egdb_pieces(4);
    engine.set_egdb_mtc_pieces(0);
    engine.params.lmp_depth = lmp_depth;
    engine.set_position(p);
    Limits limits;
    limits.depth = 4;
    limits.use_book = false;
    return engine.go(limits);
}

int run() {
    int failures = 0;
    for (const char* fen : {"B:W24,30:B1,K10", "B:W27,30:B1,K6", "B:WK1:BK6,K10"})
        for (bool reflected : {false, true}) {
            Position p;
            p.set_fen(fen);
            if (reflected) {
                const Position q = p;
                p.set(flip(q.pieces(WHITE)), flip(q.pieces(BLACK)), flip(q.kings()), ~q.side_to_move());
            }
            const egdb::Wld value = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
            if (value != egdb::Wld::Win && value != egdb::Wld::Loss) {
                std::cerr << "FAIL: fixture " << p.fen() << " is not a database win or loss\n";
                return 2;
            }
            const SearchResult on = search(p, 99), off = search(p, 0);
            if (on.score != off.score || on.best != off.best || on.nodes != off.nodes) {
                std::cerr << "FAIL: " << p.fen() << ": switching LMP changed the conversion search: score/nodes "
                          << on.score << '/' << on.nodes << " with LMP, " << off.score << '/' << off.nodes
                          << " without\n";
                ++failures;
            }
        }
    return failures == 0 ? 0 : 1;
}

}  // namespace

int main() {
    const std::string dir = test_egdb::build("conversion-lmp-test", 4, 2);
    if (dir.empty()) {
        std::cerr << "FAIL: build and load 2..4 pieces\n";
        return 2;
    }
    const int result = run();
    test_egdb::remove(dir);
    return result;
}
