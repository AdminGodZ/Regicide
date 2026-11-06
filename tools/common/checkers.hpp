#pragma once

// Self-contained English checkers referee used by the match tools (cbhost, match).
// Deliberately independent of the engine in src/, so an engine movegen bug can never
// "validate" itself in a match.
//
// Squares are PDN numbers 1..32 stored as index n-1. Black starts on 1..12 and moves
// first, toward higher square numbers; White starts on 21..32.

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cbt {

enum class Color : std::uint8_t { Black = 0, White = 1 };
constexpr Color opposite(Color c) { return c == Color::Black ? Color::White : Color::Black; }
const char* color_name(Color c);

struct Move {
    std::int8_t from = -1;  // index 0..31
    std::int8_t to = -1;
    std::uint32_t captures = 0;  // bitmask of captured squares
    std::uint8_t njumps = 0;     // number of jumps; path holds njumps landing squares
    std::array<std::int8_t, 12> path{};  // landing squares of a capture, path[njumps-1] == to

    bool is_capture() const { return captures != 0; }
    // Same from/to/captured set: identical effect on the board.
    bool same_effect(const Move& o) const { return from == o.from && to == o.to && captures == o.captures; }
};

struct Board {
    std::uint32_t black = 0, white = 0, kings = 0;
    Color stm = Color::Black;

    static Board start();
    std::uint32_t pieces(Color c) const { return c == Color::Black ? black : white; }
    std::uint32_t occupied() const { return black | white; }
    bool operator==(const Board&) const = default;

    std::vector<Move> legal_moves() const;
    Board after(const Move& m) const;
    // A move that is not a capture, not a man move: a king shuffling. CheckerBoard calls the
    // others "conversion" moves; they reset the no-progress and repetition windows.
    bool is_conversion(const Move& m) const;
    bool promotes(const Move& m) const;

    std::string fen() const;
    static std::optional<Board> from_fen(std::string_view fen, std::string* err = nullptr);

    // CheckerBoard Board8x8 conversion (board[x][y], values CB_WHITE/CB_BLACK | CB_MAN/CB_KING).
    void to_board8(int b[8][8]) const;
    static Board from_board8(const int b[8][8], Color stm);

    std::string pretty() const;
};

// "11-15", "22x15", "1x10x19" (full landing path for captures).
std::string move_to_pdn(const Move& m);

// Parse a PDN move token ("11-15", "11x18x25", "11:18") and find the legal move it denotes.
// Intermediate landing squares, if given, must match. If several legal moves share from/to
// and the text has no intermediate squares, the move is ambiguous and nullopt is returned
// (with *err set) unless `captures` disambiguates.
std::optional<Move> parse_move(const Board& b, std::string_view text, std::string* err = nullptr,
                               std::uint32_t captures = 0);

// Apply a move sequence like "9-13 21-17 5-9" (move numbers and results are skipped).
std::optional<Board> play_moves(Board b, std::string_view moves, std::vector<Move>* played = nullptr,
                                std::string* err = nullptr);

// Square helpers. sq is an index 0..31; pdn number is sq + 1.
constexpr std::uint32_t bit(int sq) { return std::uint32_t(1) << sq; }
void square_xy(int sq, int& x, int& y);  // CheckerBoard Board8x8 coordinates
int xy_square(int x, int y);             // -1 for light squares / off board

std::uint64_t perft(const Board& b, int depth);

}  // namespace cbt
