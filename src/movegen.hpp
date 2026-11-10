#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "position.hpp"

namespace regicide {

class MoveList {
public:
    static constexpr std::size_t CAPACITY = 128;

    void push(const Move& m) { moves_.m[size_++] = m; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    void clear() { size_ = 0; }
    const Move& operator[](std::size_t i) const { return moves_.m[i]; }
    Move& operator[](std::size_t i) { return moves_.m[i]; }
    const Move* begin() const { return moves_.m; }
    const Move* end() const { return moves_.m + size_; }
    Move* begin() { return moves_.m; }
    Move* end() { return moves_.m + size_; }

private:
    // Left uninitialised: a list is built at every search node, and zeroing 128 moves cost more than the
    // move generation itself.
    union Storage {
        Storage() {}
        Move m[CAPACITY];
    } moves_;
    std::size_t size_ = 0;
};

// Pieces of colour c that can move one step in direction d.
inline Bitboard movers(const Position& pos, Color c, int d) {
    const bool men_dir = c == WHITE ? d < 2 : d >= 2;
    return men_dir ? pos.pieces(c) : pos.kings(c);
}

// Pieces of colour c that have at least one capture available.
inline Bitboard capturers(const Position& pos, Color c) {
    const Bitboard opp = pos.pieces(~c), empty = pos.empty();
    Bitboard r = 0;
    for (int d = 0; d < 4; ++d) {
        const int o = opposite(d);
        r |= movers(pos, c, d) & shift(opp & shift(empty, o), o);
    }
    return r;
}

inline bool has_capture(const Position& pos, Color c) { return capturers(pos, c) != 0; }

// Destination squares of non-capturing moves for colour c (ignores compulsory capture).
inline Bitboard quiet_targets(const Position& pos, Color c) {
    const Bitboard empty = pos.empty();
    Bitboard r = 0;
    for (int d = 0; d < 4; ++d) r |= shift(movers(pos, c, d), d) & empty;
    return r;
}

// Number of non-capturing moves for colour c.
inline int quiet_move_count(const Position& pos, Color c) {
    const Bitboard empty = pos.empty();
    int n = 0;
    for (int d = 0; d < 4; ++d) n += popcount(shift(movers(pos, c, d), d) & empty);
    return n;
}

// Legal moves only. Captures are compulsory; if any exist, only captures are returned.
// Multi-jumps are generated as a single move; identical (from, to, captures) results are deduplicated.
void generate_moves(const Position& pos, MoveList& list);

// Appends all captures (possibly none) for the side to move.
void generate_captures(const Position& pos, MoveList& list);

// Parses PDN move text ("11-15", "22x15", "9x18x27") into a legal move. Intermediate squares of a capture path
// disambiguate king captures that share origin and destination. Returns false if no legal move matches.
bool parse_move(const Position& pos, const std::string& text, Move& out);

// Squares visited by a move: origin, every landing square of a multi-jump, destination.
std::vector<int> move_path(const Position& pos, const Move& m);

// PDN text of a legal move of `pos`, a capture with every square it lands on ("9x18x27"): unlike move_to_string,
// which gives the two end squares only, parse_move reads it back as the same move.
std::string move_text(const Position& pos, const Move& m);

}  // namespace regicide
