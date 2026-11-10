// Late-move reductions are per engine: an engine that searches with other LMR parameters in the middle of this
// engine's search must not change its result. The other engine runs from this one's info callback after depth 1,
// so the interleaving is deterministic; with a shared reduction table it overwrote this search's reductions.
#include <iostream>
#include <string>

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

Limits to_depth(int d) {
    Limits l;
    l.depth = d;
    l.use_book = false;
    return l;
}

}  // namespace

int main() {
    Engine isolated, a, b;
    for (Engine* e : {&isolated, &a, &b}) e->set_hash_mb(1);
    b.params.lmr_base = 4000;
    b.params.lmr_mult = 0;

    isolated.set_position(Position());
    const SearchResult expected = isolated.go(to_depth(6));

    bool called = false;
    a.set_position(Position());
    const SearchResult got = a.go(to_depth(6), [&](const SearchInfo& info) {
        if (info.depth != 1 || called) return;
        called = true;
        b.set_position(Position());
        b.go(to_depth(1));
    });
    std::cout << "isolated " << expected.score << '/' << expected.nodes << " nodes, interleaved " << got.score << '/'
              << got.nodes << " nodes\n";
    check(called, "the other engine searched during this search");
    check(got.score == expected.score && got.best == expected.best && got.nodes == expected.nodes,
          "another engine's LMR parameters do not alter this search");
    return failures == 0 ? 0 : 1;
}
