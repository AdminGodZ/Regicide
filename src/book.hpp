#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "position.hpp"

namespace regicide {

// Opening book: positions (by Zobrist key) mapped to analysed moves.
// File: "RGBK", u32 version, u32 count, then `count` sorted Entry records.
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

    // All book entries for this position.
    std::vector<Entry> lookup(const Position& pos) const;
    // Best book move, or a random one among those within `margin` of the best when margin > 0.
    std::optional<Move> probe(const Position& pos, int margin = 0, std::uint64_t seed = 0) const;

    void add(const Entry& e) { entries_.push_back(e); }
    void finalize();  // sort and deduplicate (keeps the deepest analysis per position and move)

    static Entry make_entry(const Position& pos, const Move& m, int score, int depth);
    static std::optional<Move> to_move(const Position& pos, const Entry& e);

private:
    std::vector<Entry> entries_;
};

}  // namespace regicide
