// The match runner's side of the engine protocol (match::Engine), against a scripted engine: this executable run
// with --mock or --mock-slow. No checkers engine is launched and no game is played.
//
//   engine_process_test rejected   an engine that answers a configuration command with "reply 0" fails to launch,
//                                  unless EngineSpec::allow_rejected is set
//   engine_process_test deadline   the search deadlines start when the engine is charged (after its "ok" to the
//                                  position), not when the runner sent the position: an engine that is slow to
//                                  take a position still gets its full search time
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#include "engine_process.hpp"

namespace {

// The scripted engine. Rejects every configuration command and answers the warm-up search at once. With `slow`,
// takes 60 ms to accept a position containing "SLOW" and answers later searches only when told to stop.
int mock(bool slow) {
    std::cout << "id name mock\nready\n" << std::flush;
    std::string line;
    int searches = 0;
    while (std::getline(std::cin, line)) {
        if (line == "quit") break;
        if (line.rfind("cmd ", 0) == 0) std::cout << "reply 0 rejected\n" << std::flush;
        else if (line == "newgame" || line.rfind("position ", 0) == 0) {
            if (slow && line.find("SLOW") != std::string::npos)
                std::this_thread::sleep_for(std::chrono::milliseconds(60));
            std::cout << "ok\n" << std::flush;
        } else if (line.rfind("go ", 0) == 0) {
            if (!slow || ++searches == 1) std::cout << "bestmove 11-15 result unknown time 0\n" << std::flush;
        } else if (line == "stop") std::cout << "bestmove 11-15 result unknown time 0\n" << std::flush;
    }
    return 0;
}

match::EngineSpec spec(const char* self, const char* mode) {
    match::EngineSpec s;
    s.label = "mock";
    s.argv = {self, mode};
    return s;
}

int rejected(const char* self) {
    match::EngineSpec s = spec(self, "--mock");
    s.cmds = {"set unsupported 1"};
    std::string err;
    match::Engine strict(s, "/dev/null");
    const bool refused = !strict.launch(err);
    strict.kill();
    s.allow_rejected = true;
    match::Engine allowed(s, "/dev/null");
    const bool overridden = allowed.launch(err);  // prints the runner's one-line warning
    allowed.kill();
    if (!refused) std::cerr << "FAIL: a rejected option stops the launch\n";
    if (!overridden) std::cerr << "FAIL: allow_rejected launches anyway (" << err << ")\n";
    return refused && overridden ? 0 : 1;
}

int deadline(const char* self) {
    match::Engine e(spec(self, "--mock-slow"), "/dev/null");
    std::string err;
    if (!e.launch(err)) {
        std::cerr << "FAIL: launch: " << err << '\n';
        return 1;
    }
    // Stop after 40 ms of search. The engine spends 60 ms on the position, so with deadlines counted from the
    // `position` line the stop would go out at once.
    const auto start = match::Clock::now();
    const match::GoResult r =
        e.go("position fen SLOW", "go maxtime .04", start + std::chrono::milliseconds(40), start + std::chrono::seconds(5));
    e.kill();
    std::printf("searched %.3f s\n", r.elapsed);
    if (r.status != match::GoResult::Status::Ok) std::cerr << "FAIL: the search ends with a move (" << r.detail << ")\n";
    if (r.elapsed < .03) std::cerr << "FAIL: the stop comes 40 ms after the engine took the position\n";
    return r.status == match::GoResult::Status::Ok && r.elapsed >= .03 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
    const std::string what = argc > 1 ? argv[1] : "";
    if (what == "--mock" || what == "--mock-slow") return mock(what == "--mock-slow");
    if (what == "rejected") return rejected(argv[0]);
    if (what == "deadline") return deadline(argv[0]);
    std::cerr << "usage: engine_process_test rejected|deadline\n";
    return 2;
}
