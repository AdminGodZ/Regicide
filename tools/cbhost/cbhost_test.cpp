// cbhost against a scripted CheckerBoard engine (mock_engine.cpp), and the runner's time accounting.
//
//   cbhost_test stop CBHOST ENGINE     a `stop` that arrives while the host is still sending the game history
//                                      (before the engine's getmove is entered) is not lost: the engine finds the
//                                      stop flag set on entry and returns at once
//   cbhost_test quit CBHOST ENGINE     a `quit` (or end of input) stops every go that is still queued, not only the
//                                      last one
//   cbhost_test charged CBHOST ENGINE  the runner charges a CheckerBoard time bank with the time the engine spent
//                                      in getmove, not with the host's preparation before it
//   cbhost_test incremental CBHOST ENGINE MATCH
//                                      the same in an incremental time control: an engine that answers at once but
//                                      takes 350 ms for the game history does not lose a 0.05 s game on time
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <thread>

#include "engine_process.hpp"

namespace {

using match::Clock;

bool expect(match::Process& host, const std::string& prefix, std::string& line, int seconds = 10) {
    const auto deadline = Clock::now() + std::chrono::seconds(seconds);
    while (host.read_line(line, deadline) == match::Process::Read::Line)
        if (line.rfind(prefix, 0) == 0) return true;
    return false;
}

// The engine takes 300 ms for the game history and would then think for 2 s. The stop goes out after 50 ms.
int stop(const std::string& cbhost, const std::string& engine) {
    match::Process host;
    std::string err, line, info;
    if (!host.start({cbhost, engine}, "/dev/null", err) || !expect(host, "ready", line)) {
        std::cerr << "FAIL: cbhost does not start: " << err << '\n';
        return 1;
    }
    for (const char* c : {"cmd set histdelay 300", "cmd set think 2000"}) {
        host.send(c);
        if (!expect(host, "reply 1", line)) return std::cerr << "FAIL: no reply to " << c << '\n', 1;
    }
    host.send("newgame");
    host.send("position startpos");
    if (!expect(host, "ok", line) || !expect(host, "ok", line)) return std::cerr << "FAIL: position\n", 1;
    const auto start = Clock::now();
    host.send("go maxtime 2");
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    host.send("stop");
    if (!expect(host, "info ", info) || !expect(host, "bestmove ", line)) return std::cerr << "FAIL: no move\n", 1;
    const double total = std::chrono::duration<double>(Clock::now() - start).count();
    host.send("quit");
    std::printf("%s | %s | %.3f s in all\n", info.c_str(), line.c_str(), total);
    const bool seen = info.find("stopped_on_entry 1") != std::string::npos;
    if (!seen) std::cerr << "FAIL: the stop sent during the history command reaches the engine on entry\n";
    if (total > 1.0) std::cerr << "FAIL: the stopped search returns at once (took " << total << " s)\n";
    return seen && total <= 1.0 ? 0 : 1;
}

// Two go commands and a quit are sent together; the engine takes 100 ms for each game history and would think 2 s.
int quit(const std::string& cbhost, const std::string& engine) {
    match::Process host;
    std::string err, line, info[2];
    if (!host.start({cbhost, engine}, "/dev/null", err) || !expect(host, "ready", line)) {
        std::cerr << "FAIL: cbhost does not start: " << err << '\n';
        return 1;
    }
    for (const char* c : {"cmd set histdelay 100", "cmd set think 2000"}) {
        host.send(c);
        if (!expect(host, "reply 1", line)) return std::cerr << "FAIL: no reply to " << c << '\n', 1;
    }
    host.send("newgame");
    host.send("position startpos");
    if (!expect(host, "ok", line) || !expect(host, "ok", line)) return std::cerr << "FAIL: position\n", 1;
    const auto start = Clock::now();
    host.send("go maxtime 2");
    host.send("go maxtime 2");
    host.send("quit");
    for (auto& i : info)
        if (!expect(host, "info ", i) || !expect(host, "bestmove ", line)) return std::cerr << "FAIL: no move\n", 1;
    const double total = std::chrono::duration<double>(Clock::now() - start).count();
    std::printf("%s | %s | %.3f s in all\n", info[0].c_str(), info[1].c_str(), total);
    const bool seen = info[0].find("stopped_on_entry 1") != std::string::npos &&
                      info[1].find("stopped_on_entry 1") != std::string::npos;
    if (!seen) std::cerr << "FAIL: both queued searches find the stop flag set on entry\n";
    if (total > 1.0) std::cerr << "FAIL: the stopped searches return at once (took " << total << " s)\n";
    return seen && total <= 1.0 ? 0 : 1;
}

// The engine takes 150 ms for the game history and answers the search at once.
int charged(const std::string& cbhost, const std::string& engine) {
    match::EngineSpec spec;
    spec.label = "mock";
    spec.argv = {cbhost, engine};
    spec.cmds = {"set histdelay 150"};
    match::Engine e(spec, "/dev/null");
    std::string err;
    if (!e.launch(err)) return std::cerr << "FAIL: launch: " << err << '\n', 1;
    e.new_game();
    const auto now = Clock::now();
    const match::GoResult r =
        e.go("position startpos", "go maxtime 1", now + std::chrono::seconds(5), now + std::chrono::seconds(10));
    e.kill();
    std::printf("runner %.3f s, engine %.3f s, charged %.3f s\n", r.elapsed, r.engine_time, r.charged());
    const bool ok = r.status == match::GoResult::Status::Ok && r.elapsed >= 0.14 && r.engine_time >= 0 &&
                    r.engine_time < 0.05 && r.charged() == r.engine_time;
    if (!ok) std::cerr << "FAIL: the bank is charged the engine's time, without the 150 ms history command\n";
    match::GoResult none;  // an engine that reports no time is charged the runner's wall time
    none.elapsed = 0.2;
    if (none.charged() != 0.2) return std::cerr << "FAIL: without a reported time the wall time is charged\n", 1;
    return ok ? 0 : 1;
}

// One two-ply game through the match executable, engine A with a 350 ms history command, 0.05 s on the clock.
int incremental(const std::string& cbhost, const std::string& engine, const std::string& match_exe) {
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / ("cbhost_test_" + std::to_string(::getpid()));
    fs::create_directories(dir);
    std::ofstream(dir / "opening.txt") << "B:W21-32:B1-12\n";
    const std::string spec = "exec:" + cbhost + " " + engine;
    match::Process run;
    std::string err, line;
    const bool started = run.start({match_exe, "-a", spec, "-b", spec, "--option-a", "set histdelay 350", "--openings",
                                    (dir / "opening.txt").string(), "--tc", "0.05+0", "--max-plies", "2", "-c", "1",
                                    "--out", (dir / "out").string()},
                                   "/dev/null", err);
    const auto deadline = Clock::now() + std::chrono::seconds(60);
    while (started && run.read_line(line, deadline) == match::Process::Read::Line) {
    }
    std::ifstream in(dir / "out" / "summary.txt");
    const std::string summary((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    fs::remove_all(dir);
    const bool played = summary.find("Games: 2 of 2") != std::string::npos;
    const bool forfeit = summary.find("time forfeit") != std::string::npos;
    if (!started || !played) std::cerr << "FAIL: the match plays its two games (" << err << ")\n" << summary;
    if (forfeit) std::cerr << "FAIL: the history command is not charged to the clock\n" << summary;
    return started && played && !forfeit ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string what = argc > 1 ? argv[1] : "";
    if (argc == 4 && what == "stop") return stop(argv[2], argv[3]);
    if (argc == 4 && what == "quit") return quit(argv[2], argv[3]);
    if (argc == 4 && what == "charged") return charged(argv[2], argv[3]);
    if (argc == 5 && what == "incremental") return incremental(argv[2], argv[3], argv[4]);
    std::cerr << "usage: cbhost_test stop|quit|charged CBHOST ENGINE | incremental CBHOST ENGINE MATCH\n";
    return 2;
}
