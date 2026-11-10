// A root the endgame database decides is played from the database, past the book and the avoid list: a book move
// that throws the win is not taken, and the avoid list does not push the engine off the move it would choose.
// Builds a 2..3-piece database (WLD and MTC) in a temporary directory; the engine uses only its WLD values.
#include <fcntl.h>

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <random>
#include <string>
#include <system_error>
#include <vector>

#include "book.hpp"
#include "egdb/build.hpp"
#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

#ifdef _WIN32
#include <io.h>
#define dup _dup
#define dup2 _dup2
#define open _open
#define close _close
constexpr const char* NULL_DEVICE = "NUL";
#else
#include <unistd.h>
constexpr const char* NULL_DEVICE = "/dev/null";
#endif

using namespace regicide;
using egdb::Wld;

namespace {

int failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        std::cerr << "FAIL: " << what << '\n';
        ++failures;
    }
}

Wld probe(const Position& p) { return egdb::probe(p.pieces(BLACK), p.pieces(WHITE), p.kings(), p.side_to_move()); }

// The database builder logs every slice to stdout; this keeps it quiet for the length of a scope.
struct QuietStdout {
    int saved;
    QuietStdout() {
        std::fflush(stdout);
        saved = dup(1);
        const int null = open(NULL_DEVICE, O_WRONLY);
        dup2(null, 1);
        close(null);
    }
    ~QuietStdout() {
        std::fflush(stdout);
        dup2(saved, 1);
        close(saved);
    }
};

}  // namespace

int main() {
    const std::string dir =
        (std::filesystem::temp_directory_path() / ("regicide-egdb-root-test-" + std::to_string(std::random_device{}())))
            .string();
    egdb::detail::BuildOptions b;
    b.dir = dir;
    b.threads = 1;
    b.mtc_pieces = 3;
    b.verbose = false;
    bool built;
    {
        QuietStdout quiet;
        built = egdb::detail::build_range(b, 2, 3);
    }
    check(built && egdb::init(dir, 64) && egdb::max_pieces() >= 3, "build and load 2..3 pieces in " + dir);

    // Black wins; some moves keep the win, at least one throws it.
    Position p;
    check(p.set_fen("B:WK1:BK6,K10"), "set_fen");
    check(probe(p) == Wld::Win, "the root is a database win");
    MoveList moves;
    generate_moves(p, moves);
    std::vector<Move> keep, the_rest;
    for (const Move& m : moves) {
        Position q = p;
        q.make_move(m);
        (probe(q) == Wld::Loss ? keep : the_rest).push_back(m);
    }
    check(keep.size() > 1 && !the_rest.empty(), "several moves keep the win and one does not");

    if (failures == 0) {
        Engine e;
        e.set_hash_mb(1);
        e.set_evaluator(Evaluator::hce());
        e.set_egdb_pieces(3);
        e.set_egdb_mtc_pieces(0);
        Limits l;
        l.depth = 2;

        auto book = std::make_shared<Book>();
        book->add(Book::make_entry(p, the_rest[0], 100, 12));
        book->finalize();
        e.set_book(book);
        e.set_position(p);
        const SearchResult r = e.go(l);
        Position q = p;
        q.make_move(r.best);
        check(probe(q) == Wld::Loss && r.nodes > 0, "a database win is searched, not thrown by the book move");

        e.set_book(nullptr);
        e.new_game();
        e.set_position(p);
        const Move chosen = e.go(l).best;
        auto avoid = std::make_shared<Book>();
        avoid->add_avoid(Book::make_entry(p, chosen, 0, 1));
        avoid->finalize();
        e.set_avoid(avoid);
        e.new_game();
        e.set_position(p);
        check(e.go(l).best == chosen, "a database-decided root ignores the avoid list");
    }

    std::error_code ec;
    std::filesystem::remove_all(dir, ec);  // best effort: mapped files cannot be removed on Windows
    return failures == 0 ? 0 : 1;
}
