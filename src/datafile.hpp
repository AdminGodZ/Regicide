#pragma once

#include <cstdint>

#include "position.hpp"

namespace regicide {

// One training position, 16 bytes on disk (little endian).
struct DataRecord {
    Bitboard black = 0, white = 0, kings = 0;
    std::int16_t score = 0;  // search score from the side to move's point of view
    std::uint8_t stm = 0;
    std::int8_t result = 0;  // game result from black's point of view: 1 win, 0 draw, -1 loss

    static DataRecord make(const Position& p, int score) {
        DataRecord r;
        r.black = p.pieces(BLACK);
        r.white = p.pieces(WHITE);
        r.kings = p.kings();
        r.score = std::int16_t(score);
        r.stm = p.side_to_move();
        return r;
    }

    Position position() const {
        Position p;
        p.set(black, white, kings, Color(stm));
        return p;
    }

    // Result from the side to move's point of view, mapped to 0 / 0.5 / 1.
    double stm_result() const { return 0.5 + 0.5 * (stm == BLACK ? result : -result); }
};
static_assert(sizeof(DataRecord) == 16);

}  // namespace regicide
