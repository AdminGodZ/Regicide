#include "tt.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <new>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace regicide {

namespace {

// std::aligned_alloc is missing from the Windows C runtime, whose aligned blocks need _aligned_free.
void* alloc_aligned(std::size_t align, std::size_t size) {
#ifdef _WIN32
    return _aligned_malloc(size, align);
#else
    return std::aligned_alloc(align, size);
#endif
}

void free_aligned(void* p) {
#ifdef _WIN32
    _aligned_free(p);
#else
    std::free(p);
#endif
}

}  // namespace

// data layout: move 16 | score 16 | eval 16 | depth 8 | bound 2 | generation 6
std::uint64_t TranspositionTable::pack(const TTData& d, int gen) {
    return std::uint64_t(d.move.v) | std::uint64_t(std::uint16_t(d.score)) << 16 |
           std::uint64_t(std::uint16_t(d.eval)) << 32 | std::uint64_t(std::uint8_t(d.depth + DEPTH_OFFSET)) << 48 |
           std::uint64_t(d.bound) << 56 | std::uint64_t(gen) << 58;
}

TTData TranspositionTable::unpack(std::uint64_t data) {
    TTData d;
    d.move.v = std::uint16_t(data);
    d.score = std::int16_t(data >> 16);
    d.eval = std::int16_t(data >> 32);
    d.depth = std::int8_t(int((data >> 48) & 0xFF) - DEPTH_OFFSET);
    d.bound = Bound((data >> 56) & 3);
    return d;
}

TranspositionTable::~TranspositionTable() { free_aligned(buckets_); }

// The old table is freed first, so the two never exist together. If the new size cannot be had, a table of the old
// size (or, failing that, of one bucket) is put back before the error is reported: the table is always usable.
void TranspositionTable::resize(std::size_t mb) {
    const std::size_t old = count_;
    free_aligned(buckets_);
    buckets_ = nullptr;
    bool ok = true;
    for (std::size_t count : {std::max<std::size_t>(1, mb * 1024 * 1024 / sizeof(Bucket)), old, std::size_t(1)}) {
        if (count == 0) continue;
        count_ = count;
        buckets_ = static_cast<Bucket*>(alloc_aligned(64, count_ * sizeof(Bucket)));
        if (buckets_) break;
        ok = false;
    }
    if (!buckets_) std::abort();  // not even 64 bytes
    clear();
    if (!ok) throw std::bad_alloc();
}

void TranspositionTable::clear() {
    std::memset(static_cast<void*>(buckets_), 0, count_ * sizeof(Bucket));
    generation_ = 0;
}

bool TranspositionTable::probe(Key key, TTData& out) const {
    const Bucket& b = buckets_[index(key)];
    for (const Entry& e : b.e) {
        const std::uint64_t data = e.data.load(std::memory_order_relaxed);
        if ((e.check.load(std::memory_order_relaxed) ^ data) == key && data) {
            out = unpack(data);
            return true;
        }
    }
    return false;
}

void TranspositionTable::store(Key key, int score, int eval, int depth, Bound bound, TTMove move) {
    Bucket& b = buckets_[index(key)];
    Entry* victim = nullptr;
    int worst = 1 << 30;
    for (Entry& e : b.e) {
        const std::uint64_t data = e.data.load(std::memory_order_relaxed);
        if ((e.check.load(std::memory_order_relaxed) ^ data) == key && data) {
            // Same position: keep a deeper entry from this search unless the new one is exact.
            if (bound != BOUND_EXACT && gen_of(data) == generation_ && depth_of(data) - DEPTH_OFFSET > depth + 2)
                return;
            if (!move.valid()) move.v = std::uint16_t(data);
            victim = &e;
            break;
        }
        // Prefer replacing stale and shallow entries.
        const int age = (generation_ - gen_of(data)) & 63;
        const int value = data ? depth_of(data) - 8 * age : -1000;
        if (value < worst) {
            worst = value;
            victim = &e;
        }
    }
    TTData d{move, std::int16_t(score), std::int16_t(eval), std::int8_t(depth), bound};
    const std::uint64_t data = pack(d, generation_);
    if (skip_same && victim->data.load(std::memory_order_relaxed) == data &&
        (victim->check.load(std::memory_order_relaxed) ^ data) == key)
        return;
    victim->check.store(key ^ data, std::memory_order_relaxed);
    victim->data.store(data, std::memory_order_relaxed);
}

int TranspositionTable::hashfull() const {
    int n = 0;
    const std::size_t sample = std::min<std::size_t>(count_, 250);
    for (std::size_t i = 0; i < sample; ++i)
        for (const Entry& e : buckets_[i].e) {
            const std::uint64_t data = e.data.load(std::memory_order_relaxed);
            n += data && gen_of(data) == generation_;
        }
    return int(n * 1000 / (sample * 4));
}

}  // namespace regicide
