// Avoid list: excluded root moves are never played (unless every legal move is excluded), the book never returns
// them, root-move restriction (searchmoves) works, and RGBK version 2 files round-trip.
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <string>

#include "bench.hpp"
#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

using namespace regicide;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

Position after(std::initializer_list<const char*> moves) {
    Position p;
    for (const char* t : moves) {
        Move m;
        if (!parse_move(p, t, m)) std::abort();
        p.make_move(m);
    }
    return p;
}

std::vector<Move> legal(const Position& p) {
    MoveList list;
    generate_moves(p, list);
    return {list.begin(), list.end()};
}

Move search(Engine& e, const Position& p, Limits limits) {
    e.new_game();
    e.set_position(p);
    return e.go(limits).best;
}

}  // namespace

int main() {
    Limits depth8;
    depth8.depth = 8;
    Limits timed;
    timed.movetime_ms = 20;
    Limits by_nodes;
    by_nodes.nodes = 20000;

    // 1. The engine never plays an avoided move, with one or several threads and any kind of limit.
    std::vector<Position> positions = bench_positions();
    positions.resize(8);
    for (int threads : {1, 3}) {
        Engine e;
        e.set_hash_mb(8);
        e.set_threads(threads);
        for (const Position& p : positions) {
            if (legal(p).size() < 2) continue;
            e.set_avoid(nullptr);
            const Move best = search(e, p, depth8);
            auto avoid = std::make_shared<Book>();
            avoid->add_avoid(Book::make_entry(p, best, -100, 20));
            avoid->finalize();
            check(avoid->avoids(p, best), "avoids() finds the entry");
            e.set_avoid(avoid);
            for (const Limits& l : {depth8, timed, by_nodes}) {
                const Move m = search(e, p, l);
                check(m != best && std::ranges::contains(legal(p), m),
                      "avoided move played in " + p.fen() + " (threads " + std::to_string(threads) + ")");
            }
            // Turned off: the best move is back (single-threaded searches are deterministic).
            e.set_avoid(nullptr);
            if (threads == 1) check(search(e, p, depth8) == best, "set_avoid(nullptr) restores the original choice");
        }
    }

    // 2. Every move but one avoided: that one is played, at once under a time limit.
    {
        Engine e;
        e.set_hash_mb(8);
        const Position p;
        const std::vector<Move> moves = legal(p);
        auto avoid = std::make_shared<Book>();
        for (std::size_t i = 1; i < moves.size(); ++i) avoid->add_avoid(Book::make_entry(p, moves[i], 0, 1));
        avoid->finalize();
        e.set_avoid(avoid);
        check(search(e, p, timed) == moves[0], "the only non-avoided move is played");

        // Every legal move avoided: the list is ignored and the engine still plays.
        auto all = std::make_shared<Book>(*avoid);
        all->add_avoid(Book::make_entry(p, moves[0], 0, 1));
        all->finalize();
        e.set_avoid(all);
        const Move m = search(e, p, depth8);
        check(std::ranges::contains(moves, m), "all moves avoided: a legal move is still played");

        // A single legal move that is avoided is still played.
        const Position forced = after({"9-13", "22-18", "10-14"});
        const std::vector<Move> only = legal(forced);
        check(only.size() == 1, "test position has one legal move");
        auto avoid_forced = std::make_shared<Book>();
        avoid_forced->add_avoid(Book::make_entry(forced, only[0], 0, 1));
        avoid_forced->finalize();
        e.set_avoid(avoid_forced);
        check(search(e, forced, timed) == only[0], "the only legal move is played even when avoided");
    }

    // 3. The book never returns an avoided move: neither its own avoid entries nor the engine's list.
    {
        const Position p;
        const std::vector<Move> moves = legal(p);
        Book book;
        for (std::size_t i = 0; i < moves.size(); ++i)
            book.add(Book::make_entry(p, moves[i], i == 0 ? 50 : i == 1 ? 45 : -int(i) * 10, 20));
        book.finalize();
        check(book.probe(p) == moves[0], "book plays its best move");

        Book avoid;
        avoid.add_avoid(Book::make_entry(p, moves[0], 50, 20));
        avoid.finalize();
        check(book.probe(p, 0, 0, &avoid) == moves[1], "probe skips the engine's avoided move");
        for (std::uint64_t seed = 0; seed < 32; ++seed)
            check(book.probe(p, 100, seed, &avoid) != moves[0], "random book choice skips the avoided move");

        Book own = book;
        own.add_avoid(Book::make_entry(p, moves[0], 50, 20));
        own.finalize();
        check(own.probe(p) == moves[1], "probe skips the book's own avoid entries");

        // Through the engine: the book move is replaced, and searched moves skip it as well.
        Engine e;
        e.set_hash_mb(8);
        e.set_book(std::make_shared<Book>(book));
        e.set_avoid(std::make_shared<Book>(avoid));
        for (int i = 0; i < 4; ++i) check(search(e, p, timed) == moves[1], "engine book move skips the avoided move");
        Limits no_book = timed;
        no_book.use_book = false;
        check(search(e, p, no_book) != moves[0], "engine search skips the avoided move");

        // Every book move avoided: no book move.
        Book all;
        for (const Move& m : moves) all.add_avoid(Book::make_entry(p, m, 0, 1));
        all.finalize();
        check(!book.probe(p, 0, 0, &all).has_value(), "no book move when all are avoided");
    }

    // 4. searchmoves restricts the root (lossprobe's analysis); the avoid list does not apply to it.
    {
        Engine e;
        e.set_hash_mb(8);
        const Position p;
        const std::vector<Move> moves = legal(p);
        auto avoid = std::make_shared<Book>();
        avoid->add_avoid(Book::make_entry(p, moves[2], 0, 1));
        avoid->finalize();
        e.set_avoid(avoid);
        Limits l = timed;
        l.searchmoves = {moves[2]};
        e.new_game();
        e.set_position(p);
        const SearchResult r = e.go(l);
        check(r.best == moves[2], "searchmoves: the single given move is analysed");
        check(r.depth > 3, "searchmoves: a single move is searched to the time limit, not returned at once");
        l.searchmoves = {moves[3], moves[4]};
        const Move m = search(e, p, l);
        check(m == moves[3] || m == moves[4], "searchmoves: the best of the given moves");
    }

    // 5. File format: version 1 without avoid entries (readable by older builds), version 2 with them.
    {
        const Position p;
        const std::vector<Move> moves = legal(p);
        Book book;
        book.add(Book::make_entry(p, moves[0], 10, 12));
        book.finalize();
        const auto dir = std::filesystem::temp_directory_path();
        const std::string v1 = (dir / "regicide_avoid_test_v1.bin").string();
        const std::string v2 = (dir / "regicide_avoid_test_v2.bin").string();
        check(book.save(v1), "save v1");
        check(std::filesystem::file_size(v1) == 12 + sizeof(Book::Entry), "plain book is written as version 1");
        book.add_avoid(Book::make_entry(p, moves[1], -80, 30));
        Book::Entry shallow = Book::make_entry(p, moves[1], -60, 20);  // merged: deepest kept, highest count
        shallow.count = 2;
        book.add_avoid(shallow);
        book.add_avoid(Book::make_entry(after({"11-15"}), legal(after({"11-15"}))[0], -40, 25));
        book.finalize();
        check(book.avoid_size() == 2, "duplicate avoid entries are merged");
        check(book.save(v2), "save v2");
        Book in1, in2;
        check(in1.load(v1) && in1.size() == 1 && in1.avoid_size() == 0, "load v1");
        check(in2.load(v2) && in2.size() == 1 && in2.avoid_size() == 2, "load v2");
        check(in2.avoids(p, moves[1]) && !in2.avoids(p, moves[0]), "v2 avoid entries survive a round trip");
        for (const Book::Entry& e : in2.avoid_entries())
            if (e.key == p.key()) check(e.count == 2 && e.depth == 30 && e.score == -80, "merged avoid entry");
        std::filesystem::remove(v1);
        std::filesystem::remove(v2);
    }

    // 6. The built-in book file carries no avoid entries unless one was embedded.
    std::cout << "built-in book: " << Book::builtin()->size() << " entries, " << Book::builtin()->avoid_size()
              << " avoid entries\n";

    if (failures) {
        std::cerr << failures << " failure(s)\n";
        return 1;
    }
    std::cout << "avoid: all checks passed\n";
    return 0;
}
