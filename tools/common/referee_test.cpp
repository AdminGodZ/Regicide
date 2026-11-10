// Sanity checks for the match referee: perft, FEN and Board8x8 round trips, move parsing,
// and CheckerBoard time packing.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iterator>

#include "cb_api.h"
#include "checkers.hpp"

using namespace cbt;

static int failures = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                \
        }                                                              \
    } while (0)

int main() {
    constexpr std::uint64_t expected[] = {1, 7, 49, 302, 1469, 7361, 36768, 179740, 845931, 3963680};
    for (int d = 0; d < int(std::size(expected)); ++d) {
        auto n = perft(Board::start(), d);
        std::printf("perft(%d) = %llu\n", d, (unsigned long long)n);
        CHECK(n == expected[d]);
    }

    // FEN round trip, ranges and kings.
    auto b = Board::from_fen("W:WK3,21-24:B1,K12,13.");
    CHECK(b.has_value());
    CHECK(b->stm == Color::White);
    CHECK(b->fen() == "W:WK3,21,22,23,24:B1,K12,13");
    CHECK(Board::from_fen(Board::start().fen()) == Board::start());

    // Board8x8 conversion matches CheckerBoard's numbering: square 1 at x=6,y=0, 32 at x=1,y=7.
    int b8[8][8];
    Board::start().to_board8(b8);
    CHECK(b8[6][0] == (CB_BLACK | CB_MAN));
    CHECK(b8[1][7] == (CB_WHITE | CB_MAN));
    CHECK(Board::from_board8(b8, Color::Black) == Board::start());

    // Ballot 9-13 22-18 6-9 and a capture.
    auto after = play_moves(Board::start(), "1. 9-13 22-18 2. 6-9");
    CHECK(after.has_value());
    auto caps = play_moves(Board::start(), "11-15 22-18 15x22 25x18");
    CHECK(caps.has_value());

    // Double jump with full path, and men may not capture backwards.
    auto dj = Board::from_fen("B:W15,24:B10");
    CHECK(dj && dj->legal_moves().size() == 1);
    CHECK(dj && move_to_pdn(dj->legal_moves()[0]) == "10x19x28");
    CHECK(dj && parse_move(*dj, "10x28").has_value());
    auto back = Board::from_fen("B:W6:B10");
    CHECK(back && !back->legal_moves()[0].is_capture());

    // Crowning ends the move: black man on 22 jumps 26 to land on 31 and stops.
    auto crown = Board::from_fen("B:W26,27:B22");
    CHECK(crown && crown->legal_moves().size() == 1 && crown->legal_moves()[0].njumps == 1);

    // Every legal move reads back from its own text, also a single jump that shares its end squares with a longer
    // capture around a circle.
    for (const char* fen : {"W:WK22:B9,10,17,18,25", "B:W6,7,14,15,32:BK9", "B:W6,7,14,15,32:B2",
                            "B:WK9,K10,K11,K17,K18,K19,K25,K26,K27:BK29"}) {
        auto pos = Board::from_fen(fen);
        CHECK(pos.has_value());
        for (const Move& m : pos->legal_moves()) {
            auto back = parse_move(*pos, move_to_pdn(m));
            CHECK(back.has_value() && back->same_effect(m));
        }
    }
    // The routes around a circle that take the same pieces are one move, whichever way it is written.
    auto circle = Board::from_fen("B:W6,7,14,15,32:BK9");
    auto one = parse_move(*circle, "9x2x11x18x9"), other = parse_move(*circle, "9x18x11x2x9");
    CHECK(one.has_value() && other.has_value() && one->same_effect(*other));
    CHECK(!parse_move(*circle, "9x2x9x2x11x18x9").has_value());  // jumps a piece twice

    // Time packing decodes like CheckerBoard's get_incremental_times().
    int info = 0, more = 0;
    cb_format_time_args(0.5, 61.3, &info, &more);
    int mode = (info >> CB_INCR_TIME_SHIFT) & 3;
    double unit[] = {0, 0.001, 0.01, 0.1};
    CHECK(mode == 1);
    CHECK(std::fabs((more & 0xffff) * unit[mode] - 0.5) < 1e-9);
    CHECK(std::fabs(((unsigned)more >> 16) * unit[mode] - 61.3) < 0.011);

    std::printf("%s\n", failures ? "FAILED" : "all referee checks passed");
    return failures ? 1 : 0;
}
