// The match runner's opening loader and its handling of an interrupted run. No checkers engine is launched.
//
//   runner_test comment           a parenthesis inside a PDN { } comment is text: the moves after the comment still
//                                 belong to the opening; a line starting with '%' is a comment
//   runner_test braces MATCH      an engine's status text with braces in it stays inside its PDN comment: the saved
//                                 game reads back as the moves that were played
//   runner_test fresh MATCH       every game carries the tags Worker, EngineGamesA and EngineGamesB; with
//                                 --fresh-engines each game is the first of new engine processes
//   runner_test pairs MATCH       with --pairs-together the two games of an opening pair are played one after the
//                                 other by the same worker
//   runner_test restart MATCH     when one engine dies and is restarted, only its own game counter (EngineGamesA or
//                                 EngineGamesB) starts again from 0
//   runner_test interrupt MATCH   an interrupt of the whole process group (Ctrl-C in a terminal) ends the engines
//                                 too; the game they were playing is left out of the results, not scored as a loss
//                                 by "crash". MATCH is the match executable; the engines are this executable run
//                                 with --fake.
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <thread>

#include "openings.hpp"

namespace {

namespace fs = std::filesystem;

fs::path scratch(const char* what) {
    const fs::path dir = fs::temp_directory_path() / ("regicide-" + std::string(what) + "-" + std::to_string(::getpid()));
    fs::create_directories(dir);
    return dir;
}

int comment() {
    const fs::path dir = scratch("openings");
    {
        std::ofstream f(dir / "games.pdn");
        f << "% a comment line before the first tag\n"
          << "[Event \"a % comment line in the moves\"]\n[Result \"*\"]\n\n1. 11-15\n% not a move\n22-18 *\n\n"
          << "[Event \"an unmatched ( in a comment\"]\n[Result \"*\"]\n\n1. 11-15 {an opening parenthesis ( here} 22-18 *\n\n"
          << "[Event \"an unmatched ) in a comment\"]\n[Result \"*\"]\n\n1. 11-15 {a closing one ) here} 22-18 *\n\n"
          << "[Event \"a variation\"]\n[Result \"*\"]\n\n1. 11-15 (1. 9-13 {with } ( text) 22-18) 22-18 *\n";
    }
    std::string err;
    const auto openings = match::load_openings((dir / "games.pdn").string(), err);
    fs::remove_all(dir);
    bool ok = openings.size() == 4;
    for (const auto& o : openings) {
        const bool good = o.played.size() == 2 && o.start.fen() == "B:W18,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,12,15";
        if (!good) std::cerr << "FAIL: " << o.name << ": " << o.played.size() << " moves, " << o.start.fen() << '\n';
        ok = ok && good;
    }
    // The referee's move reader: 4294967305 is 9 modulo 2^32.
    if (cbt::parse_move(cbt::Board::start(), "4294967305-13") || !cbt::parse_move(cbt::Board::start(), "9-13")) {
        std::cerr << "FAIL: a square number out of range is rejected, however long\n";
        ok = false;
    }
    if (openings.size() != 4) std::cerr << "FAIL: " << openings.size() << " openings loaded (" << err << ")\n";
    return ok ? 0 : 1;
}

// The scripted engine: answers the warm-up search; on the first search of a game it creates the file MARK and
// never answers.
int fake(const char* mark) {
    std::cout << "id name interrupt fixture\nready\n" << std::flush;
    int searches = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "newgame" || line.rfind("position ", 0) == 0)
            std::cout << "ok\n" << std::flush;
        else if (line.rfind("go ", 0) == 0) {
            if (searches++ == 0)
                std::cout << "bestmove 11-15 result unknown time 0\n" << std::flush;
            else
                std::ofstream(mark) << "entered\n";
        }
    }
    return 0;
}

// The scripted engine for `braces`: plays 11-15 from the start position and 22-18 after it, each with a status text
// that has braces and move-like words in it.
int fake_braces() {
    std::cout << "id name braces fixture\nready\n" << std::flush;
    bool moved = false;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "newgame") std::cout << "ok\n" << std::flush;
        else if (line.rfind("position ", 0) == 0) {
            moved = line.find(" moves ") != std::string::npos;
            std::cout << "ok\n" << std::flush;
        } else if (line.rfind("go ", 0) == 0)
            std::cout << "info note {X} pv 24-20 8-11 score0\nbestmove " << (moved ? "22-18" : "11-15")
                      << " result unknown time 0\n"
                      << std::flush;
    }
    return 0;
}

// The scripted engine for `restart`: plays as fake_braces does (without the status text) and exits, as a crash, at
// the first search of its second game (the warm-up's newgame counts as the first).
int fake_dies() {
    std::cout << "id name dying fixture\nready\n" << std::flush;
    int newgames = 0;
    bool moved = false;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line == "newgame") {
            ++newgames;
            std::cout << "ok\n" << std::flush;
        } else if (line.rfind("position ", 0) == 0) {
            moved = line.find(" moves ") != std::string::npos;
            std::cout << "ok\n" << std::flush;
        } else if (line.rfind("go ", 0) == 0) {
            if (newgames == 3) return 0;
            std::cout << "bestmove " << (moved ? "22-18" : "11-15") << " result unknown time 0\n" << std::flush;
        }
    }
    return 0;
}

int braces(const char* self, const char* match_exe) {
    const fs::path dir = scratch("braces");
    std::ofstream(dir / "opening.txt") << "B:W21-32:B1-12\n";
    const std::string engine = "exec:\"" + std::string(self) + "\" --fake-braces";
    const std::string command = "\"" + std::string(match_exe) + "\" -a '" + engine + "' -b '" + engine +
                                "' --openings \"" + (dir / "opening.txt").string() +
                                "\" --max-plies 2 --pdn-comments -c 1 -o \"" + (dir / "out").string() + "\" > /dev/null 2>&1";
    const int status = std::system(command.c_str());
    std::string err;
    const auto games = match::load_openings((dir / "out" / "games.pdn").string(), err);
    bool ok = status == 0 && games.size() == 2;
    for (const auto& g : games) ok = ok && g.played.size() == 2 && g.moves.find("24-20") == std::string::npos;
    if (!ok) {
        std::ifstream in(dir / "out" / "games.pdn");
        std::cerr << "FAIL: the saved games read back as two plies each (" << games.size() << " games, " << err << ")\n"
                  << std::string((std::istreambuf_iterator<char>(in)), {});
    }
    fs::remove_all(dir);
    return ok ? 0 : 1;
}

// Two games (one opening, both colours) by one worker, with and without --fresh-engines: the EngineGamesA and
// EngineGamesB tags.
int fresh(const char* self, const char* match_exe) {
    const fs::path dir = scratch("fresh");
    std::ofstream(dir / "opening.txt") << "B:W21-32:B1-12\n";
    const std::string engine = "exec:\"" + std::string(self) + "\" --fake-braces";
    auto tags = [&](const char* option, const char* out) {
        const std::string command = "\"" + std::string(match_exe) + "\" -a '" + engine + "' -b '" + engine +
                                    "' --openings \"" + (dir / "opening.txt").string() + "\" --max-plies 2 -c 1 " +
                                    option + " -o \"" + (dir / out).string() + "\" > /dev/null 2>&1";
        if (std::system(command.c_str()) != 0) return std::string("match failed");
        std::ifstream in(dir / out / "games.pdn");
        std::string line, found;
        while (std::getline(in, line))
            if (line.rfind("[EngineGames", 0) == 0 || line.rfind("[Worker ", 0) == 0) found += line;
        return found;
    };
    const std::string kept = tags("", "kept"), renewed = tags("--fresh-engines", "renewed");
    fs::remove_all(dir);
    const bool ok = kept == "[Worker \"0\"][EngineGamesA \"0\"][EngineGamesB \"0\"]"
                            "[Worker \"0\"][EngineGamesA \"1\"][EngineGamesB \"1\"]" &&
                    renewed == "[Worker \"0\"][EngineGamesA \"0\"][EngineGamesB \"0\"]"
                               "[Worker \"0\"][EngineGamesA \"0\"][EngineGamesB \"0\"]";
    if (!ok) std::cerr << "FAIL: tags without the option: " << kept << "\n      with --fresh-engines: " << renewed << '\n';
    return ok ? 0 : 1;
}

// Two openings, two workers, --pairs-together: both games of a pair carry the same Worker tag, and EngineGamesA of
// the second game is one more than of the first (the same engine process played them in a row). The scripted
// engine keeps no state, so the test cannot show that an engine carries anything from one game into the next; nor
// that the order is unchanged without the option (which worker takes which game is then a race).
int pairs(const char* self, const char* match_exe) {
    const fs::path dir = scratch("pairs");
    std::ofstream(dir / "openings.txt") << "B:W21-32:B1-12\nB:W21-31:B1-12\n";
    const std::string engine = "exec:\"" + std::string(self) + "\" --fake-braces";
    const std::string command = "\"" + std::string(match_exe) + "\" -a '" + engine + "' -b '" + engine +
                                "' --openings \"" + (dir / "openings.txt").string() +
                                "\" --max-plies 2 -c 2 --pairs-together -o \"" + (dir / "out").string() +
                                "\" > /dev/null 2>&1";
    const int status = std::system(command.c_str());
    // Round "pair.leg" -> (Worker, EngineGamesA) of that game.
    std::map<std::string, std::pair<std::string, int>> games;
    std::ifstream in(dir / "out" / "games.pdn");
    std::string line, round;
    auto value = [&] { return line.substr(line.find('"') + 1, line.rfind('"') - line.find('"') - 1); };
    while (std::getline(in, line)) {
        if (line.rfind("[Round ", 0) == 0) round = value();
        else if (line.rfind("[Worker ", 0) == 0) games[round].first = value();
        else if (line.rfind("[EngineGamesA ", 0) == 0) games[round].second = std::stoi(value());
    }
    fs::remove_all(dir);
    const std::size_t played = games.size();
    bool ok = status == 0 && played == 4;
    for (const char* pair : {"1", "2"}) {
        const auto& first = games[pair + std::string(".1")];
        const auto& second = games[pair + std::string(".2")];
        const bool same = !first.first.empty() && first.first == second.first && second.second == first.second + 1;
        if (!same)
            std::cerr << "FAIL: pair " << pair << ": worker " << first.first << " / " << second.first
                      << ", EngineGamesA " << first.second << " / " << second.second << '\n';
        ok = ok && same;
    }
    if (status != 0 || played != 4) std::cerr << "FAIL: match status " << status << ", " << played << " games\n";
    return ok ? 0 : 1;
}

// One opening, two rounds, one worker: engine A dies in the second game of each of its processes (games 1.2 and
// 2.2) while engine B lives through the match. A's counter starts again from 0 after its restart, B's goes on:
// EngineGamesA / EngineGamesB are 0/0, 1/1, 0/2, 1/3.
int restart(const char* self, const char* match_exe) {
    const fs::path dir = scratch("restart");
    std::ofstream(dir / "opening.txt") << "B:W21-32:B1-12\n";
    const std::string dies = "exec:\"" + std::string(self) + "\" --fake-dies";
    const std::string lives = "exec:\"" + std::string(self) + "\" --fake-braces";
    const std::string command = "\"" + std::string(match_exe) + "\" -a '" + dies + "' -b '" + lives +
                                "' --openings \"" + (dir / "opening.txt").string() +
                                "\" --rounds 2 --max-plies 2 -c 1 -o \"" + (dir / "out").string() + "\" > /dev/null 2>&1";
    const int status = std::system(command.c_str());
    std::ifstream in(dir / "out" / "games.pdn");
    std::string line, found;
    int crashes = 0;
    while (std::getline(in, line)) {
        if (line.rfind("[Round ", 0) == 0 || line.rfind("[EngineGames", 0) == 0) found += line;
        else if (line.rfind("[Termination ", 0) == 0 && line.find(" crashed") != std::string::npos) ++crashes;
    }
    fs::remove_all(dir);
    const std::string expected = "[Round \"1.1\"][EngineGamesA \"0\"][EngineGamesB \"0\"]"
                                 "[Round \"1.2\"][EngineGamesA \"1\"][EngineGamesB \"1\"]"
                                 "[Round \"2.1\"][EngineGamesA \"0\"][EngineGamesB \"2\"]"
                                 "[Round \"2.2\"][EngineGamesA \"1\"][EngineGamesB \"3\"]";
    const bool ok = status == 0 && crashes == 2 && found == expected;
    if (!ok)
        std::cerr << "FAIL: status " << status << ", " << crashes << " games lost by a crash (2 expected), tags:\n      "
                  << found << "\n      expected:\n      " << expected << '\n';
    return ok ? 0 : 1;
}

int interrupt(const char* self, const char* match_exe) {
    const fs::path dir = scratch("interrupt");
    std::ofstream(dir / "opening.txt") << "11-15 22-18\n";
    const std::string engine = "exec:\"" + std::string(self) + "\" --fake " + (dir / "mark").string();
    const std::string openings = (dir / "opening.txt").string(), out = (dir / "out").string();

    // The runner and its engines in a process group of their own, as a foreground job in a terminal is.
    const pid_t pid = fork();
    if (pid == 0) {
        setsid();
        std::freopen("/dev/null", "w", stdout);
        std::freopen("/dev/null", "w", stderr);
        execl(match_exe, match_exe, "-a", engine.c_str(), "-b", engine.c_str(), "--openings", openings.c_str(),
              "--pairs", "1", "-c", "1", "--movetime", "10", "-o", out.c_str(), static_cast<char*>(nullptr));
        _exit(127);
    }
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!fs::exists(dir / "mark") && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    const bool entered = fs::exists(dir / "mark");
    killpg(pid, entered ? SIGINT : SIGKILL);
    for (int i = 0; i < 1000 && waitpid(pid, nullptr, WNOHANG) == 0; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    killpg(pid, SIGKILL);
    waitpid(pid, nullptr, 0);

    auto read = [&](const char* name) {
        std::ifstream in(dir / "out" / name);
        return std::string((std::istreambuf_iterator<char>(in)), {});
    };
    const std::string results = read("results.csv"), summary = read("summary.txt");
    fs::remove_all(dir);
    const bool marked = summary.find("(interrupted)") != std::string::npos;
    const bool no_game = summary.find("Games: 0 of") != std::string::npos && results.find("crash") == std::string::npos;
    if (!entered) std::cerr << "FAIL: the game search was never reached\n";
    if (!marked) std::cerr << "FAIL: the summary does not say the run was interrupted\n" << summary;
    if (!no_game) std::cerr << "FAIL: the interrupted game was recorded as a result\n" << results << summary;
    return entered && marked && no_game ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string what = argc > 1 ? argv[1] : "";
    if (argc == 3 && what == "--fake") return fake(argv[2]);
    if (argc == 2 && what == "--fake-braces") return fake_braces();
    if (argc == 2 && what == "--fake-dies") return fake_dies();
    if (argc == 3 && what == "braces") return braces(argv[0], argv[2]);
    if (argc == 3 && what == "fresh") return fresh(argv[0], argv[2]);
    if (argc == 3 && what == "pairs") return pairs(argv[0], argv[2]);
    if (argc == 3 && what == "restart") return restart(argv[0], argv[2]);
    if (argc == 2 && what == "comment") return comment();
    if (argc == 3 && what == "interrupt") return interrupt(argv[0], argv[2]);
    std::cerr << "usage: runner_test comment | braces MATCH | fresh MATCH | pairs MATCH | restart MATCH | interrupt MATCH\n";
    return 2;
}
