// The hash table holds scores that are only valid for one evaluator and one set of database limits, so changing
// either clears it.
//
//   hash_clear_test evaluator   after Engine::set_evaluator, a search scores like a fresh engine with the new
//                               evaluator, not with the scores the old one left in the table
//   hash_clear_test egdb        Engine::set_egdb_pieces and set_egdb_mtc_pieces empty a warm table
//   hash_clear_test resize      a hash size that cannot be allocated is refused and leaves a usable table
//   hash_clear_test threads     a thread count whose workers cannot be allocated is refused and leaves the old
//                               workers in place
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <string>

#include "position.hpp"
#include "search.hpp"
#include "test_net.hpp"

using namespace regicide;

// The `threads` case makes large allocations fail on request (a search worker is several hundred kilobytes).
namespace {
std::atomic<bool> refuse_large{false};
void* allocate(std::size_t n, std::size_t align) {
    if (refuse_large && n > 100000) throw std::bad_alloc();
    align = std::max(align, sizeof(void*));
#ifdef _WIN32
    void* p = _aligned_malloc(n ? n : 1, align);
#else
    void* p = nullptr;
    if (posix_memalign(&p, align, n ? n : 1) != 0) p = nullptr;
#endif
    if (!p) throw std::bad_alloc();
    return p;
}
void release(void* p) noexcept {
#ifdef _WIN32
    _aligned_free(p);
#else
    std::free(p);
#endif
}
}  // namespace
void* operator new(std::size_t n) { return allocate(n, alignof(std::max_align_t)); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, std::size_t(a)); }
void operator delete(void* p) noexcept { release(p); }
void operator delete(void* p, std::size_t) noexcept { release(p); }
void operator delete(void* p, std::align_val_t) noexcept { release(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { release(p); }

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

void failed_resize() {
    Engine e;
    e.set_hash_mb(1);
    const Position p;
    bool refused = false;
    try {
        e.set_hash_mb(std::size_t(1) << 42);  // 2^62 bytes
    } catch (const std::bad_alloc&) {
        refused = true;
    }
    check(refused, "a hash size beyond the address space is refused");
    e.new_game();  // clears the table: it has to exist
    e.set_position(p);
    const SearchResult r = e.go(to_depth(4));
    TTData d;
    check(r.best != Move{} && e.hash().probe(p.key(), d), "the engine searches and stores after the refused size");
}

void failed_threads() {
    Engine e;
    e.set_hash_mb(1);
    bool refused = false;
    refuse_large = true;
    try {
        e.set_threads(2);
    } catch (const std::bad_alloc&) {
        refused = true;
    }
    refuse_large = false;
    check(refused, "workers that cannot be allocated are refused");
    e.set_position(Position());
    check(e.go(to_depth(4)).best != Move{}, "the engine still searches with its old workers");
}

}  // namespace

int main(int argc, char** argv) {
    const std::string what = argc > 1 ? argv[1] : "";
    if (what == "evaluator") evaluator_change();
    else if (what == "egdb") egdb_change();
    else if (what == "resize") failed_resize();
    else if (what == "threads") failed_threads();
    else {
        std::cerr << "usage: hash_clear_test evaluator|egdb|resize|threads\n";
        return 2;
    }
    return failures == 0 ? 0 : 1;
}
