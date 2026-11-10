#pragma once

#include <algorithm>
#include <cstdint>

#include "position.hpp"

namespace regicide {

// One training position, 16 bytes on disk (little endian).
struct DataRecord {
    Bitboard black = 0, white = 0, kings = 0;
    std::int16_t score = 0;  // search score from the side to move's point of view
    std::uint8_t stm = 0;    // side to move in bit 0; flags in bits 6 (GAME_START) and 7 (PROVEN), see below
    std::int8_t result = 0;  // game result from black's point of view: 1 win, 0 draw, -1 loss

    // A proven record's score is a search score that the search proved (a forced win/loss, through the endgame
    // database or by running the loser out of moves), clamped to +-PROVEN_SCORE. Its sign is the exact result.
    // GAME_START marks the first record of every game (datagen since 2026-11-10), so game boundaries need not be
    // guessed. Readers written before the flags existed must mask stm with 1; files without flags are unchanged.
    static constexpr std::uint8_t PROVEN = 0x80;
    static constexpr std::uint8_t GAME_START = 0x40;
    static constexpr int PROVEN_SCORE = 3000;

    static DataRecord make(const Position& p, int score) {
        DataRecord r;
        r.black = p.pieces(BLACK);
        r.white = p.pieces(WHITE);
        r.kings = p.kings();
        r.score = std::int16_t(score);
        r.stm = p.side_to_move();
        return r;
    }
    // A record with a proven score (`score` is a decisive search score; it is clamped and flagged).
    static DataRecord make_proven(const Position& p, int score) {
        DataRecord r = make(p, std::clamp(score, -PROVEN_SCORE, PROVEN_SCORE));
        r.stm |= PROVEN;
        return r;
    }

    Color color() const { return Color(stm & 1); }
    bool proven() const { return (stm & PROVEN) != 0; }
    bool game_start() const { return (stm & GAME_START) != 0; }
    int pieces() const { return popcount(black | white); }

    Position position() const {
        Position p;
        p.set(black, white, kings, color());
        return p;
    }

    // Result from the side to move's point of view, mapped to 0 / 0.5 / 1.
    double stm_result() const { return 0.5 + 0.5 * (color() == BLACK ? result : -result); }
};
static_assert(sizeof(DataRecord) == 16);

}  // namespace regicide
