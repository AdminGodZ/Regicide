// The no-progress count in the hash key (rev_key in src/search.cpp). In a normal search the count is left out of
// the key below 48 plies and keyed exactly from 48; in the conversion search it is keyed in steps of 16 below 48
// (0..15, 16..31, 32..47) and exactly from 48.
//
// 1. After a depth-1 search the root's entry is found under the plain position key exactly when the count is not
//    part of the key: below 48 in a normal search, below 16 in the conversion search. Counts 0, 15, 16, 31, 32, 47,
//    48 and 49, each reached by a legal history of king moves.
// 2. A static eval cached at one count is not reused at a count of another class: a warm engine that searched the
//    child at count 16 (or 48) then scores the root move into it at count 32 (or 49) exactly as a fresh engine.
// Builds a 2..3-piece database in a temporary directory; the engine uses its WLD values only.
#include <algorithm>
#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "test_egdb.hpp"

using namespace regicide;

namespace {

bool decided(const Position& p) {
    const egdb::Wld v = egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move());
    return v == egdb::Wld::Win || v == egdb::Wld::Loss;
}

// A root reached by `plies` random king moves without captures, threats of capture or a third occurrence, from an
// all-king position; for the conversion search every position after a move is a database win or loss.
bool king_run(int plies, bool conversion, Position& root, std::vector<Position>& history, int seed_offset = 0) {
    std::mt19937_64 rng(31337 + plies + 1000 * seed_offset);
    for (int attempt = 0; attempt < 1000; ++attempt) {
        Position p;
        p.set_fen(conversion ? "B:WK28:BK7,K24" : "B:WK24:BK8,K11,K22");
        std::vector<Position> h;
        for (int i = 0; i < plies; ++i) {
            MoveList moves;
            generate_moves(p, moves);
            std::vector<Move> safe;
            for (const Move& m : moves) {
                if (m.is_capture()) continue;
                Position q = p;
                q.make_move(m);
                MoveList replies;
                generate_moves(q, replies);
                if (replies.empty() || replies[0].is_capture() || std::count(h.begin(), h.end(), q) >= 2) continue;
                if (conversion && !decided(q)) continue;
                safe.push_back(m);
            }
            if (safe.empty()) break;
            h.push_back(p);
            p.make_move(safe[rng() % safe.size()]);
        }
        if (int(h.size()) == plies) {
            root = p;
            history = std::move(h);
            return true;
        }
    }
    return false;
}

// Check 2: the conversion search's eval of the root's child, cached by a warm engine at `warm_count`, is not used at
// `target_count`.
bool eval_not_shared(int warm_count, int target_count) {
    Position root, child;
    Move into_child;
    std::vector<Position> history;
    bool found = false;
    for (int attempt = 0; attempt < 100 && !found; ++attempt) {
        if (!king_run(target_count - 1, true, root, history, attempt)) continue;
        MoveList moves;
        generate_moves(root, moves);
        for (const Move& m : moves) {
            if (m.is_capture()) continue;
            Position c = root;
            c.make_move(m);
            MoveList replies;
            generate_moves(c, replies);
            if (replies.empty() || replies[0].is_capture() || std::count(history.begin(), history.end(), c) ||
                !decided(c))
                continue;
            child = c;
            into_child = m;
            found = true;
            break;
        }
    }
    if (!found) {
        std::cerr << "FAIL: no legal history for counts " << warm_count << " -> " << target_count << '\n';
        return false;
    }
    std::vector<Position> game = history;
    game.push_back(root);
    const std::vector<Position> warm_history(game.end() - warm_count, game.end());

    Engine warm, fresh;
    for (Engine* e : {&warm, &fresh}) {
        e->set_hash_mb(1);
        e->set_evaluator(Evaluator::hce());
        e->set_egdb_pieces(3);
        e->set_egdb_mtc_pieces(0);
        e->params.single_reply_ext = 0;
        e->params.threat_qext = 0;
    }
    Limits limits;
    limits.depth = 1;
    limits.use_book = false;
    warm.set_position(child, warm_history);  // caches the child's static eval at warm_count
    warm.go(limits);

    limits.searchmoves = {into_child};
    warm.set_position(root, history);
    fresh.set_position(root, history);
    const Score got = warm.go(limits).score, want = fresh.go(limits).score;
    if (got != want) {
        std::cerr << "FAIL: counts " << warm_count << " -> " << target_count << " reused an eval: " << got
                  << " instead of " << want << '\n';
        return false;
    }
    return true;
}

int run() {
    Engine engine;
    engine.set_hash_mb(1);
    engine.set_evaluator(Evaluator::hce());
    engine.set_egdb_mtc_pieces(0);
    Limits limits;
    limits.depth = 1;
    limits.use_book = false;
    int failures = 0;
    for (bool conversion : {false, true})
        for (int count : {0, 15, 16, 31, 32, 47, 48, 49}) {
            Position root;
            std::vector<Position> history;
            if (!king_run(count, conversion, root, history)) {
                std::cerr << "FAIL: no legal history of " << count << " king moves\n";
                return 2;
            }
            engine.set_egdb_pieces(conversion ? 3 : 0);
            engine.new_game();
            engine.set_position(root, history);
            engine.go(limits);
            TTData entry;
            const bool plain = engine.hash().probe(root.key(), entry);
            const bool expected = conversion ? count < 16 : count < 48;
            if (plain != expected) {
                std::cerr << "FAIL: count " << count << (conversion ? ", conversion search" : ", normal search")
                          << ": entry under the plain key " << (plain ? "found" : "not found") << '\n';
                ++failures;
            }
        }
    if (!eval_not_shared(16, 32)) ++failures;
    if (!eval_not_shared(48, 49)) ++failures;
    return failures == 0 ? 0 : 1;
}

}  // namespace

int main() {
    const std::string dir = test_egdb::build("count-key-test", 3);
    if (dir.empty()) {
        std::cerr << "FAIL: build and load 2..3 pieces\n";
        return 2;
    }
    const int result = run();
    test_egdb::remove(dir);
    return result;
}
