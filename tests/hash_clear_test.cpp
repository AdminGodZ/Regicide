// The hash table holds scores that are only valid for one evaluator and one set of database limits, so changing
// either clears it.
//
//   hash_clear_test evaluator   after Engine::set_evaluator, a search scores like a fresh engine with the new
//                               evaluator, not with the scores the old one left in the table
//   hash_clear_test egdb        Engine::set_egdb_pieces and set_egdb_mtc_pieces empty a warm table
#include <iostream>
#include <string>

#include "position.hpp"
#include "search.hpp"
#include "test_net.hpp"

using namespace regicide;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

Limits to_depth(int d) {
    Limits l;
    l.depth = d;
    l.use_book = false;
    return l;
}

void evaluator_change() {
    const auto first = test_net::constant(100), second = test_net::constant(900);
    check(first && second, "the constant nets load");
    if (!first || !second) return;

    Engine warm, fresh;
    warm.set_hash_mb(1);
    fresh.set_hash_mb(1);
    warm.set_evaluator(first);
    warm.set_position(Position());
    warm.go(to_depth(2));
    warm.set_evaluator(second);

    fresh.set_evaluator(second);
    fresh.set_position(Position());
    const Score got = warm.go(to_depth(1)).score, want = fresh.go(to_depth(1)).score;
    check(got == want, "after set_evaluator the score is the new evaluator's (" + std::to_string(got) +
                           ", a fresh engine " + std::to_string(want) + ")");
}

void egdb_change() {
    Engine e;
    e.set_hash_mb(1);
    const Position p;
    TTData d;
    e.set_position(p);
    e.go(to_depth(2));
    check(e.hash().probe(p.key(), d), "the warm table holds the root");
    e.set_egdb_pieces(3);
    check(!e.hash().probe(p.key(), d), "a change of database pieces clears the table");

    e.set_egdb_pieces(0);
    e.set_position(p);
    e.go(to_depth(2));
    check(e.hash().probe(p.key(), d), "the warm table holds the root again");
    e.set_egdb_mtc_pieces(0);
    check(!e.hash().probe(p.key(), d), "a change of MTC pieces clears the table");
}

}  // namespace

int main(int argc, char** argv) {
    const std::string what = argc > 1 ? argv[1] : "";
    if (what == "evaluator") evaluator_change();
    else if (what == "egdb") egdb_change();
    else {
        std::cerr << "usage: hash_clear_test evaluator|egdb\n";
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
