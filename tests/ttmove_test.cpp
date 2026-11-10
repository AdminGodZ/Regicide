// Hash-table move references: a move encoded from a reordered move list (the search sorts its list before it
// stores the best move) must be found again in the list as generated, also when two capture routes share their
// start and end squares.
#include <algorithm>
#include <iostream>
#include <random>
#include <string>

#include "movegen.hpp"
#include "position.hpp"
#include "tt.hpp"

using namespace regicide;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

// Encodes every move of `pos` from each rotation of the reversed move list and looks it up in the generated list.
// Returns the number of moves that share (from, to) with a different move.
int round_trip(const Position& pos) {
    MoveList gen;
    generate_moves(pos, gen);
    int shared = 0;
    for (const Move& a : gen)
        shared += std::any_of(gen.begin(), gen.end(),
                              [&](const Move& b) { return a.from == b.from && a.to == b.to && a != b; });

    MoveList perm = gen;
    std::reverse(perm.begin(), perm.end());
    for (std::size_t r = 0; r < gen.size(); ++r) {
        std::rotate(perm.begin(), perm.begin() + 1, perm.end());
        for (std::size_t i = 0; i < perm.size(); ++i) {
            const int found = find_move(gen, encode_move(perm, i));
            check(found >= 0 && gen[std::size_t(found)] == perm[i],
                  pos.fen() + ": move " + move_to_string(perm[i]) + " is not found again");
        }
    }
    return shared;
}

}  // namespace

int main() {
    // The man on 2 takes 6 and 14 or 7 and 15, ending on 18 either way.
    Position diamond;
    check(diamond.set_fen("B:W6,7,14,15,32:B2"), "set_fen");
    MoveList moves;
    generate_moves(diamond, moves);
    check(moves.size() == 2 && moves[0].from == moves[1].from && moves[0].to == moves[1].to &&
              moves[0].captures != moves[1].captures,
          "the diamond position has two capture routes with the same start and end");
    check(round_trip(diamond) == 2, "both diamond captures share (from, to)");
    check(find_move(moves, TTMove{}) == -1, "an empty reference finds no move");

    // Random games: every position on the way.
    std::mt19937 rng(7);
    int positions = 0, shared = 0;
    for (int game = 0; game < 2000; ++game) {
        Position pos;
        for (int ply = 0; ply < 150; ++ply) {
            MoveList list;
            generate_moves(pos, list);
            if (list.empty()) break;
            shared += round_trip(pos);
            ++positions;
            pos.make_move(list[rng() % list.size()]);
        }
    }
    std::cout << positions << " positions, " << shared << " moves sharing (from, to) with another\n";
    check(shared > 0, "the random games contain moves that share (from, to)");
    return failures == 0 ? 0 : 1;
}
