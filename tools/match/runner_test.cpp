// The match runner's opening loader and its handling of an interrupted run. No checkers engine is launched.
//
//   runner_test comment           a parenthesis inside a PDN { } comment is text: the moves after the comment still
//                                 belong to the opening
//   runner_test interrupt MATCH   an interrupt of the whole process group (Ctrl-C in a terminal) ends the engines
//                                 too; the game they were playing is left out of the results, not scored as a loss
//                                 by "crash". MATCH is the match executable; the engines are this executable run
//                                 with --fake.
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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
        f << "[Event \"an unmatched ( in a comment\"]\n[Result \"*\"]\n\n1. 11-15 {an opening parenthesis ( here} 22-18 *\n\n"
          << "[Event \"an unmatched ) in a comment\"]\n[Result \"*\"]\n\n1. 11-15 {a closing one ) here} 22-18 *\n\n"
          << "[Event \"a variation\"]\n[Result \"*\"]\n\n1. 11-15 (1. 9-13 {with } ( text) 22-18) 22-18 *\n";
    }
    std::string err;
    const auto openings = match::load_openings((dir / "games.pdn").string(), err);
    fs::remove_all(dir);
    bool ok = openings.size() == 3;
    for (const auto& o : openings) {
        const bool good = o.played.size() == 2 && o.start.fen() == "B:W18,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,12,15";
        if (!good) std::cerr << "FAIL: " << o.name << ": " << o.played.size() << " moves, " << o.start.fen() << '\n';
        ok = ok && good;
    }
    if (openings.size() != 3) std::cerr << "FAIL: " << openings.size() << " openings loaded (" << err << ")\n";
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
    if (argc == 2 && what == "comment") return comment();
    if (argc == 3 && what == "interrupt") return interrupt(argv[0], argv[2]);
    std::cerr << "usage: runner_test comment | interrupt MATCH\n";
    return 2;
}
