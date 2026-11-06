#include "perft.hpp"

#include <iostream>

#include "movegen.hpp"

namespace regicide {

std::uint64_t perft(const Position& pos, int depth) {
    if (depth == 0) return 1;
    MoveList list;
    generate_moves(pos, list);
    if (depth == 1) return list.size();

    std::uint64_t nodes = 0;
    for (const Move& m : list) {
        Position next = pos;
        next.make_move(m);
        nodes += perft(next, depth - 1);
    }
    return nodes;
}

std::uint64_t perft_divide(const Position& pos, int depth) {
    MoveList list;
    generate_moves(pos, list);
    std::uint64_t total = 0;
    for (const Move& m : list) {
        Position next = pos;
        next.make_move(m);
        std::uint64_t n = depth > 1 ? perft(next, depth - 1) : 1;
        std::cout << move_to_string(m) << ": " << n << '\n';
        total += n;
    }
    std::cout << "total: " << total << '\n';
    return total;
}

}  // namespace regicide
