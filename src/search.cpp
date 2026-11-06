#include "search.hpp"

#include <algorithm>
#include <chrono>
#include <iostream>

#include "movegen.hpp"

namespace regicide {

namespace {

struct Searcher {
    std::uint64_t nodes = 0;

    Score negamax(const Position& pos, int depth, int ply, Score alpha, Score beta) {
        ++nodes;
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) return -SCORE_WIN + ply;  // no moves = loss

        // Don't stop the search in the middle of a capture sequence.
        if (depth <= 0 && !list[0].is_capture()) return evaluate(pos);

        for (const Move& m : list) {
            Position next = pos;
            next.make_move(m);
            Score s = -negamax(next, depth - 1, ply + 1, -beta, -alpha);
            if (s >= beta) return s;
            alpha = std::max(alpha, s);
        }
        return alpha;
    }
};

}  // namespace

SearchResult search(const Position& pos, int max_depth) {
    SearchResult result;
    MoveList root;
    generate_moves(pos, root);
    if (root.empty()) return result;
    result.best = root[0];

    Searcher s;
    const auto start = std::chrono::steady_clock::now();

    for (int depth = 1; depth <= max_depth; ++depth) {
        Score alpha = -SCORE_INF;
        Move best = root[0];
        for (const Move& m : root) {
            Position next = pos;
            next.make_move(m);
            Score score = -s.negamax(next, depth - 1, 1, -SCORE_INF, -alpha);
            if (score > alpha) {
                alpha = score;
                best = m;
            }
        }

        // Search the previous best move first next iteration.
        for (std::size_t i = 0; i < root.size(); ++i)
            if (root[i] == best) std::swap(root[0], root[i]);

        result = {best, alpha, depth, s.nodes};
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
        std::cout << "info depth " << depth << " score " << alpha << " nodes " << s.nodes << " time " << ms
                  << " pv " << move_to_string(best) << std::endl;
    }
    return result;
}

}  // namespace regicide
