#pragma once

#include <array>
#include <cstddef>

#include "position.hpp"

namespace regicide {

class MoveList {
public:
    void push(const Move& m) { moves_[size_++] = m; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    const Move& operator[](std::size_t i) const { return moves_[i]; }
    Move& operator[](std::size_t i) { return moves_[i]; }
    const Move* begin() const { return moves_.data(); }
    const Move* end() const { return moves_.data() + size_; }

private:
    std::array<Move, 128> moves_{};
    std::size_t size_ = 0;
};

// Legal moves only. Captures are compulsory; if any exist, only captures are returned.
// Multi-jumps are generated as a single move; identical (from, to, captures) results are deduplicated.
void generate_moves(const Position& pos, MoveList& list);

}  // namespace regicide
