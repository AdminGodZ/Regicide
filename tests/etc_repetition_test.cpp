// Enhanced transposition cutoffs respect repetition: ETC does not cut off with the hash entry of a child that repeats
// a position of the game (or reaches the no-progress limit), since that entry was stored without the history.
//
// The game: B:WK27,K29:BK30, black 30-26, white 27-31. The root search covers 26-22 and 26-30; after 26-30, white's
// 31-27 returns to the first position, which the game has already seen. A depth-1 search of the first position on
// its own leaves an exact hash entry for it. With ETC from depth 1 (etc_min_depth 1) the depth-2 search must score as
// it does with ETC off; other pruning and extensions are off to keep the witness small. Builds a 2..3-piece database
// in a temporary directory.
#include <iostream>
#include <string>

#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "test_egdb.hpp"

using namespace regicide;

namespace {

SearchResult search(int etc_min_depth) {
    Engine engine;
    engine.set_hash_mb(1);
    engine.set_evaluator(Evaluator::hce());
    engine.set_egdb_pieces(3);
    engine.set_egdb_mtc_pieces(0);
    SearchParams& p = engine.params;
    p.single_reply_ext = 0;
    p.threat_qext = 0;
    p.lmr_min_depth = 99;
    p.se_depth = 99;
    p.lmp_depth = 0;
    p.fut_depth = 0;
    p.rfp_max_depth = 0;
    p.etc_min_depth = 99;

    Position first, middle, root;
    first.set_fen("B:WK27,K29:BK30");
    middle = first;
    Move m;
    if (!parse_move(middle, "30-26", m)) return {};
    middle.make_move(m);
    root = middle;
    if (!parse_move(root, "27-31", m)) return {};
    root.make_move(m);

    // An exact entry for the first position, searched without history.
    engine.set_position(first);
    Limits limits;
    limits.depth = 1;
    limits.use_book = false;
    engine.go(limits);

    engine.params.etc_min_depth = etc_min_depth;
    engine.set_position(root, {first, middle});
    limits.depth = 2;
    Move forward, back;
    parse_move(root, "26-22", forward);
    parse_move(root, "26-30", back);
    limits.searchmoves = {forward, back};
    return engine.go(limits);
}

}  // namespace

int main() {
    const std::string dir = test_egdb::build("etc-repetition-test", 3);
    if (dir.empty()) {
        std::cerr << "FAIL: build and load 2..3 pieces\n";
        return 2;
    }
    const SearchResult with_etc = search(1), without_etc = search(99);
    test_egdb::remove(dir);
    std::cout << "ETC " << with_etc.score << ", ETC off " << without_etc.score << '\n';
    if (without_etc.depth != 2 || with_etc.score != without_etc.score || with_etc.best != without_etc.best) {
        std::cerr << "FAIL: an ETC shortcut reused a repeated child's non-repeating score\n";
        return 1;
    }
    return 0;
}
