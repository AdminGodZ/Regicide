// Verifies move generation against known perft counts for English checkers.
#include <cstdint>
#include <iostream>
#include <iterator>

#include "perft.hpp"
#include "position.hpp"

using namespace regicide;

int main() {
    constexpr std::uint64_t expected[] = {1, 7, 49, 302, 1469, 7361, 36768, 179740, 845931, 3963680, 18391564};

    Position pos;
    int failures = 0;
    for (int depth = 0; depth < static_cast<int>(std::size(expected)); ++depth) {
        std::uint64_t got = perft(pos, depth);
        bool ok = got == expected[depth];
        std::cout << (ok ? "ok   " : "FAIL ") << "perft(" << depth << ") = " << got;
        if (!ok) std::cout << " (expected " << expected[depth] << ")";
        std::cout << '\n';
        failures += !ok;
    }
    return failures == 0 ? 0 : 1;
}
