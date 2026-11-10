// Pruning rules that rely on the static eval or on a shallower search stand down near the no-progress draw
// (`limit_near` in Worker::search): there every quiet line runs into the 79-ply draw, which neither the eval nor a
// reduced search sees.
//
//   near_limit_pruning_test rfp|probcut|singular
//
// Three legal 77-ply histories of king moves from B:WK24:BK8,K11,K22, in which every legal continuation up to the
// draw is capture-free, so no line resets the count and legitimately re-enables pruning. Two engines search each
// root to depth 4 with the same parameters, except that one switches the chosen rule on as aggressively as it goes
// (reverse futility at every depth with margin 0; ProbCut from depth 1 with margin 0; singular extensions from depth
// 2 with margin 0) and the other leaves it off. Near the limit the rule must not fire, so score, best move and node
// count are the same.
//
// If a deliberate search change makes this test fail, or makes it pass with the rule's `!limit_near` guard removed
// (the guard is then no longer exercised), re-derive the fixture: new histories, depth or parameters for which the
// test passes and fails without the guard. Do not delete the test.
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

// Every line of `plies` moves from p is legal, capture-free and not lost for lack of moves.
bool quiet_until_draw(const Position& p, int plies) {
    if (plies <= 0) return true;
    MoveList moves;
    generate_moves(p, moves);
    if (moves.empty()) return false;
    for (const Move& m : moves) {
        if (m.is_capture()) return false;
        Position child = p;
        child.make_move(m);
        if (!quiet_until_draw(child, plies - 1)) return false;
    }
    return true;
}

const char* const GAMES[] = {
    "11-7 24-19 22-18 19-16 7-2 16-12 8-3 12-16 18-15 16-12 15-18 12-16 3-8 16-20 18-23 20-24 8-4 "
    "24-20 4-8 20-24 23-26 24-19 26-31 19-15 31-27 15-10 27-24 10-14 24-19 14-17 8-4 17-14 19-24 "
    "14-9 24-27 9-5 27-24 5-1 24-28 1-5 28-24 5-1 24-27 1-5 27-31 5-9 31-27 9-14 2-7 14-17 27-32 "
    "17-14 32-27 14-17 27-24 17-14 24-28 14-17 7-10 17-22 10-7 22-25 4-8 25-21 8-4 21-17 28-32 "
    "17-14 7-2 14-17 32-28 17-21 28-32 21-17 32-28 17-13 2-7 ",
    "8-12 24-27 11-7 27-31 22-17 31-27 17-13 27-31 12-8 31-26 8-4 26-30 13-17 30-25 7-10 25-29 "
    "10-7 29-25 7-2 25-30 2-7 30-26 17-13 26-30 4-8 30-26 7-2 26-23 13-17 23-27 8-4 27-32 17-21 "
    "32-27 4-8 27-32 2-6 32-28 21-25 28-32 6-10 32-27 8-4 27-23 10-7 23-18 7-11 18-14 11-7 14-18 "
    "7-3 18-14 25-29 14-18 3-8 18-15 8-12 15-11 29-25 11-15 25-21 15-11 21-25 11-15 25-22 15-11 "
    "22-17 11-15 17-13 15-18 13-9 18-23 9-14 23-26 14-9 26-30 9-5 ",
    "22-26 24-19 8-4 19-24 11-7 24-19 7-10 19-24 26-30 24-28 10-7 28-24 4-8 24-27 7-2 27-24 8-11 "
    "24-19 11-8 19-24 8-3 24-28 3-8 28-32 30-25 32-27 25-29 27-32 8-11 32-27 11-16 27-32 16-11 "
    "32-28 11-8 28-24 8-3 24-27 2-7 27-31 29-25 31-27 25-29 27-24 7-2 24-19 2-6 19-15 29-25 15-11 "
    "6-1 11-16 25-30 16-12 1-6 12-16 6-2 16-12 30-25 12-16 25-29 16-12 2-6 12-16 6-9 16-11 29-25 "
    "11-16 25-29 16-20 9-13 20-24 13-9 24-19 9-6 19-23 6-2 ",
};

}  // namespace

int main(int argc, char** argv) {
    const std::string mode = argc > 1 ? argv[1] : "rfp";
    Engine enabled, disabled;
    for (Engine* engine : {&enabled, &disabled}) {
        engine->set_hash_mb(1);
        engine->set_evaluator(Evaluator::hce());
        SearchParams& p = engine->params;
        p.single_reply_ext = 0;
        p.threat_qext = 0;
        p.lmp_depth = 0;
        p.rfp_max_depth = 0;
        p.fut_depth = 0;
        p.razor_depth = 0;
        p.lmr_min_depth = 99;
        p.se_depth = 99;
    }
    if (mode == "rfp") {
        enabled.params.rfp_max_depth = 99;
        enabled.params.rfp_margin = 0;
    } else if (mode == "probcut") {
        enabled.params.probcut_depth = 1;
        enabled.params.probcut_margin = 0;
        enabled.params.probcut_reduction = 2;
    } else if (mode == "singular") {
        enabled.params.se_depth = 2;
        enabled.params.se_margin = 0;
    } else {
        std::cerr << "usage: near_limit_pruning_test rfp|probcut|singular\n";
        return 2;
    }

    for (const char* game : GAMES) {
        Position root;
        root.set_fen("B:WK24:BK8,K11,K22");
        std::vector<Position> history;
        std::istringstream input(game);
        for (std::string text; input >> text;) {
            Move move;
            if (!parse_move(root, text, move)) {
                std::cerr << "FAIL: fixture: " << text << " is not legal in " << root.fen() << '\n';
                return 2;
            }
            history.push_back(root);
            root.make_move(move);
        }
        if (history.size() != 77 || !quiet_until_draw(root, 2)) {
            std::cerr << "FAIL: fixture: " << root.fen() << " is not 77 plies in with only quiet lines to the draw\n";
            return 2;
        }
        Limits limits;
        limits.depth = 4;
        limits.use_book = false;
        enabled.new_game();
        disabled.new_game();
        enabled.set_position(root, history);
        disabled.set_position(root, history);
        const SearchResult got = enabled.go(limits), want = disabled.go(limits);
        if (got.score != want.score || got.best != want.best || got.nodes != want.nodes) {
            std::cerr << "FAIL: " << mode << " fired near the no-progress limit in " << root.fen()
                      << ": expected score " << want.score << ", " << want.nodes << " nodes (rule off); observed score "
                      << got.score << ", " << got.nodes << " nodes (rule on)\n";
            return 1;
        }
    }
    return 0;
}
