#pragma once

#include <array>
#include <string>

#include "types.hpp"

namespace regicide {

namespace zobrist {

// [piece][square] with piece = 2 * color + is_king, plus a side-to-move key.
inline constexpr auto PSQ = [] {
    std::array<std::array<Key, 32>, 4> t{};
    Key s = 0x9E3779B97F4A7C15ull;
    for (auto& row : t)
        for (Key& k : row) {
            s += 0x9E3779B97F4A7C15ull;  // splitmix64
            Key z = s;
            z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
            z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
            k = z ^ (z >> 31);
        }
    return t;
}();
inline constexpr Key SIDE = 0xF1E2D3C4B5A69788ull;

}  // namespace zobrist

class Position {
public:
    static constexpr const char* START_FEN =
        "B:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12";

    Position() { set_fen(START_FEN); }

    // PDN FEN, e.g. "W:WK3,21,22:B1,5,K30". Returns false on malformed input.
    bool set_fen(const std::string& fen);
    std::string fen() const;
    // Sets the board directly; the reversible-move counter starts at 0.
    void set(Bitboard black, Bitboard white, Bitboard kings, Color stm);

    void make_move(const Move& m);

    Bitboard pieces(Color c) const { return pieces_[c]; }
    Bitboard men(Color c) const { return pieces_[c] & ~kings_; }
    Bitboard kings(Color c) const { return pieces_[c] & kings_; }
    Bitboard kings() const { return kings_; }
    Bitboard occupied() const { return pieces_[BLACK] | pieces_[WHITE]; }
    Bitboard empty() const { return ~occupied(); }
    Color side_to_move() const { return stm_; }
    Key key() const { return key_; }
    int piece_count() const { return popcount(occupied()); }
    // Plies since the last capture or man move (only king shuffles are reversible).
    int reversible_plies() const { return reversible_; }
    void set_reversible_plies(int n) { reversible_ = std::uint16_t(n); }

    // Key of the position after `m`, without making it (for TT prefetch / ETC).
    Key key_after(const Move& m) const;

    bool operator==(const Position& o) const {
        return pieces_[0] == o.pieces_[0] && pieces_[1] == o.pieces_[1] && kings_ == o.kings_ && stm_ == o.stm_;
    }

    std::string pretty() const;

private:
    Key compute_key() const;

    Bitboard pieces_[2] = {0, 0};
    Bitboard kings_ = 0;
    Color stm_ = BLACK;
    std::uint16_t reversible_ = 0;
    Key key_ = 0;
};

std::string move_to_string(const Move& m);

}  // namespace regicide
