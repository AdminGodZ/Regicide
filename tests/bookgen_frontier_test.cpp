// bookgen's frontier counts a transposition with its cheapest path (Dijkstra over regret), not with whichever path
// a depth-first walk reached first.
//
// Two roots, 9-13 22-18 12-16 (a) and 12-16 24-20 9-13 (b), meet in one analysed position t (a 24-20, b 22-18).
// From a that move costs a regret of 20000 (a's other move is scored 20000); from b it is the best move (regret 0)
// and b's four other moves cost 20, 30, 40 and 50. The roots file lists b before a, so the old walk reached t from
// a first. With --expansions 4, bookgen must expand t's leaf and not b's fourth alternative. The scores are made up
// to isolate the frontier order; nothing is searched beyond one node per position.
//
//   bookgen_frontier_test BOOKGEN    the bookgen executable
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "book.hpp"
#include "engine_process.hpp"
#include "movegen.hpp"
#include "position.hpp"

using namespace regicide;

namespace {

Position after(std::initializer_list<const char*> moves) {
    Position p;
    for (const char* text : moves) {
        Move m;
        if (!parse_move(p, text, m)) throw std::runtime_error(text);
        p.make_move(m);
    }
    return p;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: bookgen_frontier_test BOOKGEN\n";
        return 2;
    }
    const Position a = after({"9-13", "22-18", "12-16"}), b = after({"12-16", "24-20", "9-13"});
    Move a_to_t, b_to_t;
    if (!parse_move(a, "24-20", a_to_t) || !parse_move(b, "22-18", b_to_t)) return 2;
    Position t = a, via_b = b;
    t.make_move(a_to_t);
    via_b.make_move(b_to_t);
    if (!(t == via_b)) {
        std::cerr << "FAIL: fixture: the two routes do not transpose\n";
        return 2;
    }

    MoveList a_moves, b_moves, t_moves;
    generate_moves(a, a_moves);
    generate_moves(b, b_moves);
    generate_moves(t, t_moves);
    const Move a_other = *std::find_if(a_moves.begin(), a_moves.end(), [&](const Move& m) { return m != a_to_t; });
    std::vector<Move> b_others;
    for (const Move& m : b_moves)
        if (m != b_to_t) b_others.push_back(m);
    if (b_others.size() < 4) return 2;
    Position leaf = t, competitor = b;
    leaf.make_move(t_moves[0]);
    competitor.make_move(b_others[3]);

    Book book;
    book.add(Book::make_entry(a, a_to_t, 0, 1));
    book.add(Book::make_entry(a, a_other, 20000, 1));
    book.add(Book::make_entry(b, b_to_t, 0, 1));
    for (int i = 0; i < 4; ++i) book.add(Book::make_entry(b, b_others[i], -20 - 10 * i, 1));
    book.add(Book::make_entry(t, t_moves[0], 0, 1));
    book.finalize();

    const auto dir =
        std::filesystem::temp_directory_path() /
        ("regicide-frontier-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    book.save((dir / "resume.bin").string());
    {
        std::ofstream f(dir / "roots.txt");
        f << b.fen() << '\n' << a.fen() << '\n';  // the old depth-first walk took the expensive route first
    }

    match::Process bookgen;
    std::string err;
    bool ok =
        bookgen.start({argv[1], "--resume", (dir / "resume.bin").string(), "--roots", (dir / "roots.txt").string(),
                       "--nodes", "1", "--threads", "1", "--expansions", "4", "--out", (dir / "out.bin").string()},
                      "/dev/null", err);
    std::string line, output;
    const auto deadline = match::Clock::now() + std::chrono::seconds(30);
    if (ok)
        while (bookgen.read_line(line, deadline) == match::Process::Read::Line) output += line + "\n";
    bookgen.kill();

    Book result;
    ok = ok && result.load((dir / "out.bin").string()) && !result.lookup(leaf).empty() &&
         result.lookup(competitor).empty();
    if (!ok)
        std::cerr << "FAIL: the frontier did not expand the cheapest transposition path " << err << "\nleaf "
                  << leaf.fen() << ", competitor " << competitor.fen() << "\nbookgen output:\n"
                  << output;
    std::filesystem::remove_all(dir);
    return ok ? 0 : 1;
}
