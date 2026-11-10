// Search contracts: the no-progress draw, what a search of only some root moves may leave in the hash table, and
// the completed-iteration fields of SearchResult.
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

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

Position fen(const std::string& f) {
    Position p;
    if (!p.set_fen(f)) std::abort();
    return p;
}

Limits to_depth(int d) {
    Limits l;
    l.depth = d;
    l.use_book = false;
    return l;
}

}  // namespace

int main() {
    // No-progress draw: three kings against one win easily, but after 78 plies of king moves every quiet move
    // completes the 79 plies that end the game as a draw.
    {
        const Position root = fen("B:WK20:BK6,K9,K11");
        Engine engine;
        engine.set_position(root);
        const Score fresh = engine.go(to_depth(10)).score;
        check(fresh > 100, "three kings against one are winning (score " + std::to_string(fresh) + ")");

        // 78 earlier all-king positions with the same piece count; none can come up again in the search.
        std::vector<Position> history;
        for (int i = 0; i < 78; ++i)
            history.push_back(fen("B:WK" + std::to_string(13 + i % 20) + ":BK1,K2,K" + std::to_string(3 + i / 20)));
        engine.new_game();
        engine.set_position(root, history);
        const Score late = engine.go(to_depth(10)).score;
        check(late == SCORE_DRAW, "the 79th ply without progress is a draw (score " + std::to_string(late) + ")");

        // Close to the limit, with no capture in reach, every line runs into the draw, also on a hash table
        // warmed up by a search of the same position with a fresh count.
        const Position quiet = fen("B:WK24:BK8,K11,K22");
        history.resize(75);
        engine.new_game();
        engine.set_position(quiet);
        const Score early = engine.go(to_depth(10)).score;
        engine.set_position(quiet, history);
        const Score warm = engine.go(to_depth(6)).score;
        check(early > 100 && warm == SCORE_DRAW,
              "four plies before the limit a quiet position is a draw on a warm table (fresh count " +
                  std::to_string(early) + ", near the limit " + std::to_string(warm) + ")");
    }

    // A search of some root moves must not leave an exact score or an upper bound for the position in the hash
    // table, and its move reference must be the one a full move list decodes.
    {
        Position start;
        MoveList all;
        generate_moves(start, all);
        Engine engine;
        Limits l = to_depth(6);
        l.searchmoves = {all[all.size() - 1]};
        engine.set_position(start);
        engine.go(l);
        TTData e;
        check(engine.hash().probe(start.key(), e), "the restricted search stored the root");
        check(e.bound == BOUND_LOWER, "a restricted root is stored as a lower bound");
        check(find_move(all, e.move) == int(all.size()) - 1, "the stored move is the searched one");

        const Position diamond = fen("B:W6,7,14,15,32:B2");
        MoveList routes;
        generate_moves(diamond, routes);
        for (std::size_t i = 0; i < routes.size(); ++i) {
            engine.new_game();
            l.searchmoves = {routes[i]};
            engine.set_position(diamond);
            const SearchResult r = engine.go(l);
            check(r.best == routes[i], "the only searched capture route is played");
            check(engine.hash().probe(diamond.key(), e) && find_move(routes, e.move) == int(i),
                  "capture route " + std::to_string(i) + " searched alone is found again among both");
        }
    }

    // Result fields.
    {
        Engine engine;
        engine.set_position(Position());
        const SearchResult full = engine.go(to_depth(8));
        check(!full.partial && full.depth == 8 && full.completed_score == full.score,
              "a depth-limited search ends on a completed iteration");
        int partial = 0;
        for (std::uint64_t nodes : {3000, 5000, 8000, 13000, 21000, 34000}) {
            Limits l;
            l.nodes = nodes;
            l.use_book = false;
            engine.new_game();
            const SearchResult r = engine.go(l);
            partial += r.partial;
            check(r.depth >= 1 && r.completed_score != -SCORE_INF, "a few thousand nodes complete an iteration");
            if (!r.partial) check(r.score == r.completed_score, "without a partial iteration the scores agree");
        }
        std::cout << partial << " of 6 node-limited searches ended inside an iteration\n";
    }
    return failures == 0 ? 0 : 1;
}
