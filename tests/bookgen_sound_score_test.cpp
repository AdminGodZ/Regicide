// bookgen stores a move's score from SearchResult::sound_score, so a cut-short search that only saw losing lines
// does not become a proven-loss claim in the book.
//
// W:WK5,K13,K25,29:BK10,K17 has one move, a capture. With a net that scores every position +100 for the side to
// move, an 8192-node search of the position after it ends in a partial iteration with a losing score (-29982) whose
// sound score is not decisive (-100). The test checks that the fixture still behaves so (a search change may need a
// new fixture), then runs bookgen on the parent with the same net and budget: the book must hold +100, not +29982.
//
//   bookgen_sound_score_test BOOKGEN    the bookgen executable
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "book.hpp"
#include "engine_process.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"
#include "test_net.hpp"

using namespace regicide;

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: bookgen_sound_score_test BOOKGEN\n";
        return 2;
    }
    Position parent;
    parent.set_fen("W:WK5,K13,K25,29:BK10,K17");
    MoveList moves;
    generate_moves(parent, moves);
    if (moves.size() != 1) return 2;
    Position child = parent;
    child.make_move(moves[0]);

    const auto dir =
        std::filesystem::temp_directory_path() /
        ("regicide-book-score-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    auto net = test_net::bytes();
    test_net::put<std::int32_t>(net, test_net::OUT_BIAS, 2032);  // +100 for the side to move
    {
        std::ofstream f(dir / "net.nnue", std::ios::binary);
        f.write(reinterpret_cast<const char*>(net.data()), std::streamsize(net.size()));
    }

    Engine engine;
    engine.set_evaluator(Evaluator::from_file((dir / "net.nnue").string()));
    engine.set_position(child);
    Limits limits;
    limits.nodes = 8192;
    limits.use_book = false;
    const SearchResult result = engine.go(limits);
    if (!(result.partial && result.score <= -SCORE_KNOWN_WIN && !is_decisive(result.sound_score()))) {
        std::cerr << "FAIL: the partial-loss fixture changed: score " << result.score << ", sound score "
                  << result.sound_score() << ", partial " << result.partial << '\n';
        std::filesystem::remove_all(dir);
        return 2;
    }

    {
        std::ofstream f(dir / "roots.txt");
        f << parent.fen() << '\n';
    }
    match::Process bookgen;
    std::string err;
    bool ok =
        bookgen.start({argv[1], "--roots", (dir / "roots.txt").string(), "--evalfile", (dir / "net.nnue").string(),
                       "--nodes", "8192", "--threads", "1", "--expansions", "0", "--out", (dir / "book.bin").string()},
                      "/dev/null", err);
    std::string line;
    const auto deadline = match::Clock::now() + std::chrono::seconds(30);
    if (ok)
        while (bookgen.read_line(line, deadline) == match::Process::Read::Line) {
        }
    bookgen.kill();

    Book book;
    ok = ok && book.load((dir / "book.bin").string());
    bool found = false;
    for (const Book::Entry& entry : book.lookup(parent)) {
        const auto m = Book::to_move(parent, entry);
        if (m && *m == moves[0]) {
            found = true;
            ok &= entry.score == -result.sound_score();
            std::cout << "book " << entry.score << ", sound " << -result.sound_score() << ", raw " << -result.score
                      << '\n';
        }
    }
    std::filesystem::remove_all(dir);
    if (!ok || !found) std::cerr << "FAIL: bookgen stored a partial losing score as a book claim " << err << '\n';
    return ok && found ? 0 : 1;
}
