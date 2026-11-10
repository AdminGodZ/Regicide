#pragma once

// A child process speaking the cbhost line protocol (tools/cbhost/PROTOCOL.md).

#include <sys/types.h>

#include <chrono>
#include <string>
#include <vector>

namespace match {

using Clock = std::chrono::steady_clock;

class Process {
public:
    Process() = default;
    Process(const Process&) = delete;
    Process& operator=(const Process&) = delete;
    ~Process() { kill(); }

    // Start argv[0] with stdin/stdout piped to us and stderr appended to stderr_path.
    bool start(const std::vector<std::string>& argv, const std::string& stderr_path, std::string& err);
    bool send(const std::string& line);

    enum class Read { Line, Timeout, Eof };
    Read read_line(std::string& line, Clock::time_point deadline);

    void kill();
    bool running() const { return pid_ > 0; }

private:
    pid_t pid_ = -1;
    int to_child_ = -1, from_child_ = -1;
    std::string buf_;
};

struct EngineSpec {
    std::string label;                // "A" / "B"
    std::string name;                 // display name; empty = take it from the engine
    std::vector<std::string> argv;    // process to run
    std::vector<std::string> cmds;    // enginecommand strings sent after startup ("set hashsize 64")
    bool allow_rejected = false;      // go on when the engine rejects one of `cmds`
};

// Search result of one `go`.
struct GoResult {
    enum class Status { Ok, Illegal, Crash, Timeout, Error } status = Status::Error;
    std::string move;       // PDN, as sent by the engine
    std::string claim;      // engine's own result claim: unknown/win/loss/draw
    std::string info;       // engine status string
    std::string detail;     // error description
    double elapsed = 0;     // wall time measured by the runner, from sending `go` to reading the move
    double engine_time = -1;  // time inside the engine's getmove as the host reports it ("time S"); -1 = not reported

    // What a CheckerBoard time bank is charged: the engine's own thinking time, as CheckerBoard measures it around
    // getmove. The runner's wall time also holds the host's preparation (sending the game history) and the pipe, which
    // differ from engine to engine; it is the fallback for engines that report no time, and an upper bound.
    double charged() const { return engine_time >= 0 && engine_time < elapsed ? engine_time : elapsed; }
};

class Engine {
public:
    explicit Engine(EngineSpec spec, std::string log_path) : spec_(std::move(spec)), log_path_(std::move(log_path)) {}

    // Launch, handshake, send configuration commands and a warm-up search (engines load nets,
    // books and databases lazily on their first search).
    bool launch(std::string& err);
    bool restart(std::string& err) {
        proc_.kill();
        return launch(err);
    }
    bool alive() const { return proc_.running(); }
    void kill() { proc_.kill(); }

    bool new_game();
    // Search: `position` and `go` lines are passed verbatim. `stop_at`: send `stop`;
    // `kill_at`: give up (the process is killed and the result is a timeout).
    GoResult go(const std::string& position, const std::string& go, Clock::time_point stop_at, Clock::time_point kill_at);

    const std::string& name() const { return name_; }

private:
    bool expect(const std::string& prefix, std::string& line, double seconds);

    EngineSpec spec_;
    std::string log_path_;
    std::string name_;
    Process proc_;
};

}  // namespace match
