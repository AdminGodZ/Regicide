// booklab_override (WP12): one candidate = one effective change of one book key, applied to an immutable copy of
// the base book, with the file size unchanged (so the candidate DLL is the base DLL with the embedded book bytes
// replaced, nothing else).
//
//   booklab_override BASE OUT FEN MOVE
// The book move at FEN becomes MOVE:
//   - if the key has a single entry (a forced move, e.g. a WP11 pin): that entry's move becomes MOVE, its score and
//     depth are kept;
//   - if it has several entries (natural book): if MOVE is one of them its score becomes the key's best score + 1,
//     otherwise the entry of the current book move is replaced by MOVE with that score.
// Refused: no entry for the key, MOVE illegal or on the avoid list, a protected decision is not checked here (the
// selection step excludes r10's decision positions).
// Check after writing: OUT has the same size as BASE, every entry of every other key is byte-identical, OUT's probe at
// FEN is MOVE and BASE's was not. Prints: "changed KEY BEFORE -> MOVE (entries N)".
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <tuple>
#include <vector>

#include "book.hpp"
#include "movegen.hpp"
#include "movetext.hpp"
#include "position.hpp"

using namespace regicide;

static std::vector<unsigned char> read_all(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

int main(int argc, char** argv) {
    if (argc != 5) {
        std::cerr << "usage: booklab_override BASE OUT FEN MOVE\n";
        return 2;
    }
    const std::vector<unsigned char> base = read_all(argv[1]);
    Book bk;
    if (base.size() < 12 || !bk.load(base.data(), base.size())) {
        std::cerr << "cannot load " << argv[1] << '\n';
        return 1;
    }
    Position pos;
    if (!pos.set_fen(argv[3])) {
        std::cerr << "bad FEN\n";
        return 1;
    }
    // MOVE is written as booklab::move_text writes it (full jump path), as in the candidate lists
    MoveList list;
    generate_moves(pos, list);
    std::vector<Move> ms;
    for (const Move& m : list)
        if (booklab::move_text(pos, m) == argv[4]) ms.push_back(m);
    if (ms.size() != 1) {
        std::cerr << "move " << argv[4] << ": " << ms.size() << " legal moves with this text\n";
        return 1;
    }
    const Move mv = ms[0];
    if (bk.avoids(pos, mv)) {
        std::cerr << "move is on the avoid list\n";
        return 1;
    }
    const auto before = bk.probe(pos);
    if (!before) {
        std::cerr << "no book move at this key\n";
        return 1;
    }
    if (*before == mv) {
        std::cerr << "the book already plays this move\n";
        return 1;
    }
    std::uint32_t version = 0, count = 0;
    std::memcpy(&version, base.data() + 4, 4);
    std::memcpy(&count, base.data() + 8, 4);
    const std::size_t header = version == 2 ? 16 : 12;
    auto entry_at = [&](std::vector<unsigned char>& d, std::size_t i) {
        return reinterpret_cast<Book::Entry*>(d.data() + header + i * sizeof(Book::Entry));
    };
    std::vector<unsigned char> out = base;
    const Book::Entry want = Book::make_entry(pos, mv, 0, 0);
    std::vector<std::size_t> idx;
    for (std::size_t i = 0; i < count; ++i)
        if (entry_at(out, i)->key == pos.key()) idx.push_back(i);
    if (idx.empty()) {
        std::cerr << "no entry for the key\n";
        return 1;
    }
    int best = -32768;
    for (auto i : idx) best = std::max(best, int(entry_at(out, i)->score));
    auto same = [](const Book::Entry& a, const Book::Entry& b) { return a.from == b.from && a.to == b.to && a.disc == b.disc; };
    Book::Entry* target = nullptr;
    if (idx.size() == 1) {
        target = entry_at(out, idx[0]);
    } else {
        for (auto i : idx)
            if (same(*entry_at(out, i), want)) target = entry_at(out, i);
        if (!target)
            for (auto i : idx)
                if (auto m = Book::to_move(pos, *entry_at(out, i)); m && *m == *before) target = entry_at(out, i);
        if (!target) {
            std::cerr << "cannot find the entry of the current book move\n";
            return 1;
        }
        if (best >= 32767) {
            std::cerr << "best score saturated\n";
            return 1;
        }
        target->score = std::int16_t(best + 1);
    }
    target->from = want.from, target->to = want.to, target->disc = want.disc;
    // Entries are sorted by key, then by move inside a key (finalize()); several entries of one key may now be out of
    // order. Re-sort the key's run with the same order as Book::finalize: (key, from, to, disc).
    if (idx.size() > 1) {
        std::vector<Book::Entry> run;
        for (auto i : idx) run.push_back(*entry_at(out, i));
        std::sort(run.begin(), run.end(), [](const Book::Entry& a, const Book::Entry& b) {
            return std::tie(a.from, a.to, a.disc) < std::tie(b.from, b.to, b.disc);
        });
        for (std::size_t j = 0; j < idx.size(); ++j) *entry_at(out, idx[j]) = run[j];
    }
    {
        std::ofstream f(argv[2], std::ios::binary);
        f.write(reinterpret_cast<const char*>(out.data()), std::streamsize(out.size()));
    }
    // checks
    const std::vector<unsigned char> back = read_all(argv[2]);
    Book ob;
    if (back.size() != base.size() || !ob.load(back.data(), back.size())) {
        std::cerr << "check failed: size or load\n";
        return 1;
    }
    std::vector<unsigned char> b2 = base;
    for (std::size_t i = 0; i < count; ++i) {
        const auto* x = entry_at(b2, i);
        const auto* y = reinterpret_cast<const Book::Entry*>(back.data() + header + i * sizeof(Book::Entry));
        if (std::find(idx.begin(), idx.end(), i) == idx.end() && std::memcmp(x, y, sizeof(Book::Entry)) != 0) {
            std::cerr << "check failed: entry " << i << " of another key changed\n";
            return 1;
        }
    }
    if (std::memcmp(base.data(), back.data(), header) != 0 ||
        std::memcmp(base.data() + header + count * sizeof(Book::Entry), back.data() + header + count * sizeof(Book::Entry),
                    base.size() - header - count * sizeof(Book::Entry)) != 0) {
        std::cerr << "check failed: header or avoid list changed\n";
        return 1;
    }
    const auto after = ob.probe(pos);
    if (!after || !(*after == mv)) {
        std::cerr << "check failed: the new book does not play the move\n";
        return 1;
    }
    std::cout << "changed " << std::hex << pos.key() << std::dec << ' ' << booklab::move_text(pos, *before) << " -> "
              << booklab::move_text(pos, mv) << " (entries " << idx.size() << ")\n";
    return 0;
}
