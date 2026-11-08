#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "position.hpp"

namespace regicide {

// Opening book: positions (by Zobrist key) mapped to analysed moves, plus an avoid list of (position, move) pairs
// the engine must never play at the root (learned from lost games, see tools/lossprobe.cpp).
// File version 1: "RGBK", u32 1, u32 count, then `count` sorted Entry records.
// File version 2: "RGBK", u32 2, u32 count, u32 avoid_count, then the book entries, then `avoid_count` sorted Entry
// records for the avoid list (score/depth: the analysis that blamed the move; count: lost games that blamed it in one lossprobe run).
// save() writes version 1 when the avoid list is empty, so plain books stay readable by older builds.
class Book {
public:
    struct Entry {
        Key key;
        std::uint8_t from, to;
        std::uint8_t disc;  // index among legal moves sharing (from, to), see TTMove
        std::uint8_t depth;
        std::int16_t score;  // from the side to move's point of view
        std::uint16_t count;
    };
    static_assert(sizeof(Entry) == 16);

    bool load(const std::string& path);
    bool load(const unsigned char* data, std::size_t size);
    // The book compiled into the binary (empty if none was embedded at build time).
    static std::shared_ptr<const Book> builtin();
    bool save(const std::string& path) const;
    bool empty() const { return entries_.empty(); }
    std::size_t size() const { return entries_.size(); }
    std::size_t avoid_size() const { return avoid_.size(); }
    const std::vector<Entry>& avoid_entries() const { return avoid_; }

    // All book entries for this position.
    std::vector<Entry> lookup(const Position& pos) const;
    // Best book move, or a random one among those within `margin` of the best when margin > 0. Moves on this
    // book's avoid list or on `avoid` (if given) are never returned.
    std::optional<Move> probe(const Position& pos, int margin = 0, std::uint64_t seed = 0,
                              const Book* avoid = nullptr) const;
    // Is `m` on the avoid list in this position?
    bool avoids(const Position& pos, const Move& m) const;

    void add(const Entry& e) { entries_.push_back(e); }
    void add_avoid(const Entry& e) { avoid_.push_back(e); }
    void finalize();  // sort and deduplicate (keeps the deepest analysis per position and move)

    static Entry make_entry(const Position& pos, const Move& m, int score, int depth);
    static std::optional<Move> to_move(const Position& pos, const Entry& e);

private:
    std::vector<Entry> entries_;
    std::vector<Entry> avoid_;
};

}  // namespace regicide
