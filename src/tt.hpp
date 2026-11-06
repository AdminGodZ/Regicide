#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>

#include "types.hpp"

namespace regicide {

enum Bound : std::uint8_t { BOUND_NONE = 0, BOUND_UPPER = 1, BOUND_LOWER = 2, BOUND_EXACT = 3 };

// Compact move reference: from, to and the index among moves sharing (from, to) in generation order
// (only king multi-captures can share them). Zero means "no move".
struct TTMove {
    std::uint16_t v = 0;
    static TTMove make(int from, int to, int disc) { return {std::uint16_t(1 | from << 1 | to << 6 | disc << 11)}; }
    bool valid() const { return v != 0; }
    int from() const { return (v >> 1) & 31; }
    int to() const { return (v >> 6) & 31; }
    int disc() const { return (v >> 11) & 3; }
};

struct TTData {
    TTMove move;
    std::int16_t score = 0;
    std::int16_t eval = 0;
    std::int8_t depth = 0;
    Bound bound = BOUND_NONE;
};

// Shared, lock-free transposition table. Each entry is stored as (key ^ data, data) so a torn write by
// another thread fails the key check instead of returning garbage.
class TranspositionTable {
public:
    static constexpr int DEPTH_OFFSET = 32;  // stored depth = depth + offset, allows small negative depths

    ~TranspositionTable();
    void resize(std::size_t mb);
    void clear();
    void new_search() { generation_ = (generation_ + 1) & 63; }

    bool probe(Key key, TTData& out) const;
    void store(Key key, int score, int eval, int depth, Bound bound, TTMove move);
    void prefetch(Key key) const { __builtin_prefetch(&buckets_[index(key)]); }
    int hashfull() const;  // permille of entries from the current search

private:
    struct Entry {
        std::atomic<std::uint64_t> check{0};  // key ^ data
        std::atomic<std::uint64_t> data{0};
    };
    struct alignas(64) Bucket {
        Entry e[4];
    };

    std::size_t index(Key key) const { return std::size_t((__uint128_t(key) * count_) >> 64); }
    static std::uint64_t pack(const TTData& d, int gen);
    static TTData unpack(std::uint64_t data);
    static int gen_of(std::uint64_t data) { return int(data >> 58); }
    static int depth_of(std::uint64_t data) { return int((data >> 48) & 0xFF); }

    Bucket* buckets_ = nullptr;
    std::size_t count_ = 0;
    int generation_ = 0;
};

}  // namespace regicide
