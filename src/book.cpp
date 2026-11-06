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
    char magic[4];
    std::uint32_t version = 0, count = 0;
    bool ok = std::fread(magic, 1, 4, f) == 4 && std::memcmp(magic, "RGBK", 4) == 0 &&
              std::fread(&version, 4, 1, f) == 1 && version == 1 && std::fread(&count, 4, 1, f) == 1;
    if (ok) {
        entries_.resize(count);
        ok = std::fread(entries_.data(), sizeof(Entry), count, f) == count;
    }
    std::fclose(f);
    if (!ok) entries_.clear();
    return ok;
}

bool Book::save(const std::string& path) const {
    std::FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    const std::uint32_t version = 1, count = std::uint32_t(entries_.size());
    std::fwrite("RGBK", 1, 4, f);
    std::fwrite(&version, 4, 1, f);
    std::fwrite(&count, 4, 1, f);
    std::fwrite(entries_.data(), sizeof(Entry), entries_.size(), f);
    return std::fclose(f) == 0;
}

void Book::finalize() {
    std::sort(entries_.begin(), entries_.end(), [](const Entry& a, const Entry& b) {
        if (a.key != b.key) return a.key < b.key;
        if (a.from != b.from || a.to != b.to || a.disc != b.disc)
            return std::tie(a.from, a.to, a.disc) < std::tie(b.from, b.to, b.disc);
        return a.depth > b.depth;
    });
    entries_.erase(std::unique(entries_.begin(), entries_.end(),
                               [](const Entry& a, const Entry& b) {
                                   return a.key == b.key && a.from == b.from && a.to == b.to && a.disc == b.disc;
                               }),
                   entries_.end());
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

std::optional<Move> Book::probe(const Position& pos, int margin, std::uint64_t seed) const {
    std::vector<Entry> es = lookup(pos);
    if (es.empty()) return std::nullopt;
    std::sort(es.begin(), es.end(), [](const Entry& a, const Entry& b) { return a.score > b.score; });
    std::size_t n = 1;
    while (margin > 0 && n < es.size() && es[n].score >= es[0].score - margin) ++n;
    return to_move(pos, es[seed % n]);
}

}  // namespace regicide
