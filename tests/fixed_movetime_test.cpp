// A fixed time per move is used in full: the time manager's early stop on a stable best move (at 0.8 or 0.5 of the
// budget) applies to a time budget, not to Limits::movetime_ms. A constant evaluator keeps the best move stable,
// and the info callback holds depth 3 until 135 ms of the 160 ms budget, past the old 0.8 threshold (128 ms).
// The search must then go on to the hard limit. A loaded machine can only make the search take longer.
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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

}  // namespace

int main() {
    const auto flat = test_net::constant(0);
    check(flat != nullptr, "the constant net loads");
    if (!flat) return 1;
    Engine e;
    e.set_hash_mb(1);
    e.set_evaluator(flat);
    e.set_position(Position());
    Limits l;
    l.movetime_ms = 160;
    l.use_book = false;
    const auto start = std::chrono::steady_clock::now();
    const SearchResult r = e.go(l, [&](const SearchInfo& info) {
        if (info.depth == 3) std::this_thread::sleep_until(start + std::chrono::milliseconds(135));
    });
    std::cout << r.time_ms << " ms, depth " << r.depth << '\n';
    check(r.time_ms >= 150, "a fixed move time is not cut short by a stable best move (" +
                                std::to_string(r.time_ms) + " ms of 160)");
    return failures == 0 ? 0 : 1;
}
