// Position::set_fen and parse_move: square lists, ranges, kings, capture paths, and input that must be rejected
// instead of half-read.
#include <iostream>
#include <string>

#include "movegen.hpp"
#include "position.hpp"

using namespace regicide;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

// The position `fen` parses to, as a normalised FEN; "" if it is rejected.
std::string parsed(const std::string& fen) {
    Position p;
    return p.set_fen(fen) ? p.fen() : "";
}

}  // namespace

int main() {
    const std::string start = Position().fen();
    check(parsed(Position::START_FEN) == start, "the start position round-trips");
    check(parsed("B:W21-32:B1-12") == start, "ranges give the start position");
    check(parsed("B:W21-24,25,26-32:B1-12") == start, "ranges and single squares mix");
    check(parsed("B:W21-32:B1-12\r") == start, "a trailing carriage return is ignored");
    check(parsed("B:W21, 22:B1 ,2") == parsed("B:W21,22:B1,2"), "spaces around squares are ignored");
    check(parsed("W:WK1,K2:BK32") == parsed("W:WK1-2:BK32"), "a king range");
    check(parsed("W:WK1,K2:BK32") != parsed("W:W1,2:BK32") && !parsed("W:WK1,K2:BK32").empty(), "kings differ from men");

    for (const char* bad : {"B:W21x:B1", "B:W0:B1", "B:W33:B1", "B:W5-3:B1", "B:W21-:B1", "B:W21-40:B1", "B:W-5:B1",
                            "B:W21-22-23:B1", "B:W2a:B1", "B:W5:B5", "X:W5:B1"})
        check(parsed(bad).empty(), std::string("rejected: ") + bad);

    // parse_move: a path of more than two squares has to be a chain of jumps, and picks its capture route.
    const Position opening;
    Move m;
    check(parse_move(opening, "11-15", m), "11-15 is a move");
    check(!parse_move(opening, "11x1x15", m) && !parse_move(opening, "11-1-15", m), "impossible paths are rejected");
    Position diamond;
    check(diamond.set_fen("B:W6,7,14,15,32:B2"), "set_fen");
    Move left, right;
    check(parse_move(diamond, "2x9x18", left) && parse_move(diamond, "2x11x18", right) && left != right,
          "a full path picks its capture route");
    return failures == 0 ? 0 : 1;
}
