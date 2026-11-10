// lossprobe takes a losing score from a cut-short search as evidence about the moves it belongs to only: the
// alternatives that were not yet searched fall back to their last completed score (SearchResult::sound_score), so a
// position is not declared already lost because the played move is.
//
// B:WK5,K22,K25,29:BK10, black to move, with a net that scores every position +100 for the side to move. An
// 8192-node search of all moves ends in a partial iteration with a losing score (-29982) whose sound score is not
// decisive (-100) and whose best move is not 10-6; a search of 10-6 alone proves it lost. The test checks that the
// fixture still behaves so (a search change may need a new fixture), then runs lossprobe on a one-move game in which
// black played 10-6 and lost: the report must blame ply 0 (not call the position already lost), and the avoid list
// must hold 10-6. This tests what the bounded searches established, not the position's game-theoretic value. No
// book is involved: lossprobe and the searches here run with use_book off.
//
//   lossprobe_partial_loss_test LOSSPROBE    the lossprobe executable
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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
        std::cerr << "usage: lossprobe_partial_loss_test LOSSPROBE\n";
        return 2;
    }
    const auto dir =
        std::filesystem::temp_directory_path() /
        ("regicide-lossprobe-partial-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    auto net = test_net::bytes();
    test_net::put<std::int32_t>(net, test_net::OUT_BIAS, 2032);  // +100 for the side to move
    {
        std::ofstream file(dir / "net.nnue", std::ios::binary);
        file.write(reinterpret_cast<const char*>(net.data()), std::streamsize(net.size()));
    }

    Position root;
    root.set_fen("B:WK5,K22,K25,29:BK10");
    Move played;
    if (!parse_move(root, "10-6", played)) {
        std::filesystem::remove_all(dir);
        return 2;
    }
    // lossprobe's two searches: every move, then the played move alone (it is not the best move of the first).
    Engine engine;
    engine.set_hash_mb(64);
    engine.set_evaluator(Evaluator::from_file((dir / "net.nnue").string()));
    engine.new_game();
    engine.set_position(root);
    MoveList moves;
    generate_moves(root, moves);
    Limits limits;
    limits.nodes = 8192;
    limits.use_book = false;
    limits.searchmoves.assign(moves.begin(), moves.end());
    const SearchResult all_moves = engine.go(limits);
    limits.searchmoves = {played};
    engine.set_position(root);
    const SearchResult played_alone = engine.go(limits);
    if (!all_moves.partial || all_moves.score > -SCORE_KNOWN_WIN || is_decisive(all_moves.sound_score()) ||
        all_moves.best == played || played_alone.score > -SCORE_KNOWN_WIN) {
        std::cerr << "FAIL: the partial-loss fixture changed: all moves raw " << all_moves.score << ", sound "
                  << all_moves.sound_score() << ", partial " << all_moves.partial << "; 10-6 alone "
                  << played_alone.score << '\n';
        std::filesystem::remove_all(dir);
        return 2;
    }

    {
        std::ofstream file(dir / "game.pdn");
        file << "[Black \"Regicide\"]\n[White \"fixture\"]\n[Result \"0-1\"]\n[FEN \"" << root.fen()
             << "\"]\n1. 10-6 0-1\n";
    }
    match::Process lossprobe;
    std::string error;
    bool ok =
        lossprobe.start({argv[1], "--pdn", (dir / "game.pdn").string(), "--nodes", "8192", "--hash", "64", "--jobs",
                         "1", "--threads", "1", "--ballot", "0", "--evalfile", (dir / "net.nnue").string(), "--report",
                         (dir / "report.txt").string(), "--out", (dir / "avoid.bin").string()},
                        "/dev/null", error);
    std::string line;
    const auto deadline = match::Clock::now() + std::chrono::seconds(30);
    if (ok)
        while (lossprobe.read_line(line, deadline) == match::Process::Read::Line) {
        }
    lossprobe.kill();

    Book avoid;
    ok = ok && avoid.load((dir / "avoid.bin").string()) && avoid.avoids(root, played);
    std::ifstream file(dir / "report.txt");
    const std::string report((std::istreambuf_iterator<char>(file)), {});
    ok = ok && report.find("BLAME ply 0") != std::string::npos &&
         report.find("already lost at ply 0") == std::string::npos;
    if (!ok)
        std::cerr << "FAIL: partial losses of the alternatives became a proof that the position was lost " << error
                  << "\nlossprobe report:\n"
                  << report;
    std::filesystem::remove_all(dir);
    return ok ? 0 : 1;
}
