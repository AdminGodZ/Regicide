#include "bench.hpp"

#include <chrono>
#include <cstdint>
#include <iostream>

#include "movegen.hpp"

namespace regicide {

std::vector<Position> bench_positions() {
    std::vector<Position> out{Position()};
    std::uint64_t rng = 0x5EED5EED12345678ull;
    auto next = [&] {
        rng ^= rng << 13;
        rng ^= rng >> 7;
        rng ^= rng << 17;
        return rng;
    };
    for (int plies : {6, 8, 10, 12, 14, 16, 18, 20, 24, 28, 32, 36, 40, 44, 50, 56}) {
        // Retry until the playout survives the requested number of plies.
        for (;;) {
            Position p;
            bool ok = true;
            for (int i = 0; i < plies && ok; ++i) {
                MoveList list;
                generate_moves(p, list);
                if (list.empty()) ok = false;
                else p.make_move(list[next() % list.size()]);
            }
            MoveList list;
            generate_moves(p, list);
            if (ok && !list.empty()) {
                out.push_back(p);
                break;
            }
        }
    }
    return out;
}

void run_bench(Engine& engine, int depth) {
    if (depth <= 0) depth = 16;
    std::uint64_t nodes = 0;
    std::int64_t ms = 0;
    for (const Position& p : bench_positions()) {
        engine.new_game();
        engine.set_position(p);
        Limits limits;
        limits.depth = depth;
        SearchResult r = engine.go(limits);
        nodes += r.nodes;
        ms += r.time_ms;
        std::cout << p.fen() << "  " << move_to_string(r.best) << " " << r.score << " " << r.nodes << '\n';
    }
    std::cout << "bench: " << nodes << " nodes " << ms << " ms " << nodes * 1000 / std::max<std::int64_t>(1, ms)
              << " nps" << std::endl;
}

}  // namespace regicide
