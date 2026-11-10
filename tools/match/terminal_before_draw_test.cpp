// The match runner scores a move that leaves the opponent without a move as a win, before its draw rules: with
// --no-progress 1 and --max-plies 1 the winning king move would otherwise also end the game as a draw.
//
// The opening B:WK29:BK18,K25 is played twice with colours reversed by two scripted engines (this executable run
// with --fake): black plays 18-22, after which white has no move. Both games must be black wins ("no legal moves"),
// none a draw. No checkers engine is launched.
//
//   terminal_before_draw_test MATCH    the match executable
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

#include "engine_process.hpp"

namespace {

// The scripted engine: answers the warm-up search with 11-15 and every later search with 18-22.
int fake() {
    std::cout << "id name terminal fixture\nready\n" << std::flush;
    int searches = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "newgame" || line.rfind("position ", 0) == 0)
            std::cout << "ok\n" << std::flush;
        else if (line.rfind("go ", 0) == 0)
            std::cout << "bestmove " << (searches++ == 0 ? "11-15" : "18-22") << " result unknown time 0\n"
                      << std::flush;
    }
    return 0;
}

// An exec: engine spec that runs this executable with --fake.
std::string command(const char* self) {
    std::string spec = "exec:\"";
    for (char c : std::string(self)) {
        if (c == '\\' || c == '"') spec += '\\';
        spec += c;
    }
    return spec + "\" --fake";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--fake") return fake();
    if (argc != 2) {
        std::cerr << "usage: terminal_before_draw_test MATCH\n";
        return 2;
    }
    const auto dir =
        std::filesystem::temp_directory_path() /
        ("regicide-terminal-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directory(dir);
    {
        std::ofstream f(dir / "opening.txt");
        f << "B:WK29:BK18,K25\n";
    }

    match::Process runner;
    std::string err;
    bool ok = runner.start(
        {argv[1], "-a", command(argv[0]), "-b", command(argv[0]), "--openings", (dir / "opening.txt").string(),
         "--no-progress", "1", "--max-plies", "1", "-c", "1", "-o", (dir / "out").string()},
        "/dev/null", err);
    std::string line;
    const auto deadline = match::Clock::now() + std::chrono::seconds(30);
    if (ok)
        while (runner.read_line(line, deadline) == match::Process::Read::Line) {
        }
    runner.kill();

    std::ifstream in(dir / "out/results.csv");
    const std::string results((std::istreambuf_iterator<char>(in)), {});
    int black_wins = 0;
    for (std::size_t at = 0; (at = results.find(",1-0,", at)) != std::string::npos; at += 5) ++black_wins;
    ok = ok && black_wins == 2 && results.find("1/2-1/2") == std::string::npos &&
         results.find("no legal moves") != std::string::npos;
    std::filesystem::remove_all(dir);
    if (!ok)
        std::cerr << "FAIL: a move that leaves the opponent without a move was not scored as a win " << err << '\n';
    return ok ? 0 : 1;
}
