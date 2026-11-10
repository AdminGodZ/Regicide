#include "book.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <tuple>

#include "movegen.hpp"

namespace regicide {

bool Book::load(const std::string& path) {
    std::FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return false;
    std::vector<unsigned char> data;
    unsigned char buf[65536];
    for (std::size_t n; (n = std::fread(buf, 1, sizeof buf, f)) > 0;) data.insert(data.end(), buf, buf + n);
    std::fclose(f);
    return load(data.data(), data.size());
}

bool Book::load(const unsigned char* data, std::size_t size) {
    entries_.clear();
    avoid_.clear();
    std::uint32_t version = 0, count = 0, avoid_count = 0;
    if (size < 12 || std::memcmp(data, "RGBK", 4) != 0) return false;
    std::memcpy(&version, data + 4, 4);
    std::memcpy(&count, data + 8, 4);
    if (version == 2) {
        if (size < 16) return false;
        std::memcpy(&avoid_count, data + 12, 4);
    } else if (version != 1) {
        return false;
    }
    const std::size_t header = version == 2 ? 16 : 12;
    if (size < header + (std::size_t(count) + avoid_count) * sizeof(Entry)) return false;
    entries_.resize(count);
    std::memcpy(entries_.data(), data + header, std::size_t(count) * sizeof(Entry));
    avoid_.resize(avoid_count);
    std::memcpy(avoid_.data(), data + header + std::size_t(count) * sizeof(Entry), avoid_count * sizeof(Entry));
    return true;
}

namespace {

constexpr unsigned char EMBEDDED_BOOK[] = {
    0,
#embed "weights/book.bin"
};

}  // namespace

std::shared_ptr<const Book> Book::builtin() {
    static const std::shared_ptr<const Book> b = [] {
        auto book = std::make_shared<Book>();
        book->load(EMBEDDED_BOOK + 1, sizeof(EMBEDDED_BOOK) - 1);
        return book;
    }();
    return b;
}

bool Book::save(const std::string& path) const {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const std::uint32_t version = avoid_.empty() ? 1 : 2, count = std::uint32_t(entries_.size()),
                        avoid_count = std::uint32_t(avoid_.size());
    std::fwrite("RGBK", 1, 4, f);
    std::fwrite(&version, 4, 1, f);
    std::fwrite(&count, 4, 1, f);
    if (version == 2) std::fwrite(&avoid_count, 4, 1, f);
    std::fwrite(entries_.data(), sizeof(Entry), entries_.size(), f);
    std::fwrite(avoid_.data(), sizeof(Entry), avoid_.size(), f);
    return std::fclose(f) == 0;
}

namespace {

bool same_move(const Book::Entry& a, const Book::Entry& b) {
    return a.key == b.key && a.from == b.from && a.to == b.to && a.disc == b.disc;
}

// Sorts by position and move, deepest analysis first.
void sort_entries(std::vector<Book::Entry>& es) {
    std::sort(es.begin(), es.end(), [](const Book::Entry& a, const Book::Entry& b) {
        if (a.key != b.key) return a.key < b.key;
        if (a.from != b.from || a.to != b.to || a.disc != b.disc)
            return std::tie(a.from, a.to, a.disc) < std::tie(b.from, b.to, b.disc);
        return a.depth > b.depth;
    });
}

}  // namespace

void Book::finalize() {
    sort_entries(entries_);
    entries_.erase(std::unique(entries_.begin(), entries_.end(), same_move), entries_.end());
    // Avoid entries: one per position and move, with the deepest analysis and the highest blame count.
    sort_entries(avoid_);
    std::vector<Entry> merged;
    for (const Entry& e : avoid_) {
        if (!merged.empty() && same_move(merged.back(), e))
            merged.back().count = std::max(merged.back().count, e.count);
        else
            merged.push_back(e);
    }
    avoid_ = std::move(merged);
}

Book::Entry Book::make_entry(const Position& pos, const Move& m, int score, int depth) {
    MoveList list;
    generate_moves(pos, list);
    int disc = 0;
    for (const Move& x : list) {
        if (x == m) break;
        disc += x.from == m.from && x.to == m.to;
    }
    return {pos.key(), m.from, m.to, std::uint8_t(disc), std::uint8_t(std::clamp(depth, 0, 255)),
            std::int16_t(score), 1};
}

std::optional<Move> Book::to_move(const Position& pos, const Entry& e) {
    MoveList list;
    generate_moves(pos, list);
    int disc = e.disc;
    for (const Move& m : list)
        if (m.from == e.from && m.to == e.to && disc-- == 0) return m;
    return std::nullopt;
}

std::vector<Book::Entry> Book::lookup(const Position& pos) const {
    auto lo = std::lower_bound(entries_.begin(), entries_.end(), pos.key(),
                               [](const Entry& e, Key k) { return e.key < k; });
    std::vector<Entry> out;
    for (; lo != entries_.end() && lo->key == pos.key(); ++lo) out.push_back(*lo);
    return out;
}

bool Book::avoids(const Position& pos, const Move& m) const {
    auto it = std::lower_bound(avoid_.begin(), avoid_.end(), pos.key(),
                               [](const Entry& e, Key k) { return e.key < k; });
    for (; it != avoid_.end() && it->key == pos.key(); ++it)
        if (it->from == m.from && it->to == m.to && to_move(pos, *it) == m) return true;
    return false;
}

std::optional<Move> Book::probe(const Position& pos, int margin, std::uint64_t seed, const Book* avoid) const {
    std::vector<Entry> es = lookup(pos);
    // Only entries that name a legal move which is not on an avoid list take part in the choice.
    std::erase_if(es, [&](const Entry& e) {
        const std::optional<Move> m = to_move(pos, e);
        return !m || avoids(pos, *m) || (avoid && avoid->avoids(pos, *m));
    });
    if (es.empty()) return std::nullopt;
    std::sort(es.begin(), es.end(), [](const Entry& a, const Entry& b) { return a.score > b.score; });
    std::size_t n = 1;
    while (margin > 0 && n < es.size() && es[n].score >= es[0].score - margin) ++n;
    return to_move(pos, es[seed % n]);
}

}  // namespace regicide
