#pragma once

#include <string>

#include "types.hpp"

namespace regicide {

class Position {
public:
    static constexpr const char* START_FEN =
        "B:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12";

    Position() { set_fen(START_FEN); }

    // PDN FEN, e.g. "W:WK3,21,22:B1,5,K30". Returns false on malformed input.
    bool set_fen(const std::string& fen);
    std::string fen() const;

    void make_move(const Move& m);

    Bitboard pieces(Color c) const { return pieces_[c]; }
    Bitboard kings() const { return kings_; }
    Bitboard occupied() const { return pieces_[BLACK] | pieces_[WHITE]; }
    Bitboard empty() const { return ~occupied(); }
    Color side_to_move() const { return stm_; }

    std::string pretty() const;

private:
    Bitboard pieces_[2] = {0, 0};
    Bitboard kings_ = 0;
    Color stm_ = BLACK;
};

std::string move_to_string(const Move& m);

}  // namespace regicide
