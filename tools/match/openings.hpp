#pragma once

#include <string>
#include <vector>

#include "checkers.hpp"

namespace match {

struct Opening {
    std::string name;             // "ACF #12" or "file:7"
    std::string moves;            // moves from `fen` (or from the start position), PDN
    std::string fen;              // empty = standard start position
    cbt::Board start;             // position after `moves`: where the engines take over
    std::vector<cbt::Move> played;  // `moves`, parsed
};

// Deck names: "acf156" (ACF 3-move deck without the 18 barred ballots: 144 cross-board + 12
// mail-play; the usual tournament set), "acf174" (all ballots including barred), "acf144"
// (cross-board only; CheckerBoard's default), or a path to a file. A file may contain
//  - one opening per line: a FEN ("B:W21-32:B1-12"), a move sequence from the start
//    position ("9-13 22-18 6-9"), or "FEN | moves"; '#' starts a comment;
//  - or PDN games: each game's FEN tag (if any) and movetext form one opening.
std::vector<Opening> load_openings(const std::string& deck, std::string& err);

}  // namespace match
