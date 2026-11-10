#include "engine_process.hpp"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <sstream>

extern char** environ;

namespace match {

bool Process::start(const std::vector<std::string>& argv, const std::string& stderr_path, std::string& err) {
    kill();
    // Close-on-exec from the start, so pipes of other games never leak into this child (a
    // leaked write end would hide a dead engine's EOF). dup2 in the child clears the flag.
    int in_pipe[2], out_pipe[2];
#ifdef __linux__
    bool ok = pipe2(in_pipe, O_CLOEXEC) == 0 && pipe2(out_pipe, O_CLOEXEC) == 0;
#else
    bool ok = pipe(in_pipe) == 0 && pipe(out_pipe) == 0;
    if (ok)
        for (int fd : {in_pipe[0], in_pipe[1], out_pipe[0], out_pipe[1]}) fcntl(fd, F_SETFD, FD_CLOEXEC);
#endif
    if (!ok) {
        err = std::string("pipe: ") + std::strerror(errno);
        return false;
    }

    posix_spawn_file_actions_t fa;
    posix_spawn_file_actions_init(&fa);
    posix_spawn_file_actions_adddup2(&fa, in_pipe[0], STDIN_FILENO);
    posix_spawn_file_actions_adddup2(&fa, out_pipe[1], STDOUT_FILENO);
    posix_spawn_file_actions_addopen(&fa, STDERR_FILENO, stderr_path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
#ifdef POSIX_SPAWN_CLOEXEC_DEFAULT
    // macOS: also closes fds opened by other threads between pipe() and fcntl().
    posix_spawnattr_setflags(&attr, POSIX_SPAWN_CLOEXEC_DEFAULT);
#endif

    std::vector<char*> args;
    for (const auto& a : argv) args.push_back(const_cast<char*>(a.c_str()));
    args.push_back(nullptr);
    pid_t pid;
    int rc = posix_spawn(&pid, args[0], &fa, &attr, args.data(), environ);
    posix_spawn_file_actions_destroy(&fa);
    posix_spawnattr_destroy(&attr);
    close(in_pipe[0]);
    close(out_pipe[1]);
    if (rc != 0) {
        close(in_pipe[1]);
        close(out_pipe[0]);
        err = "cannot start " + argv[0] + ": " + std::strerror(rc);
        return false;
    }
    pid_ = pid;
    to_child_ = in_pipe[1];
    from_child_ = out_pipe[0];
    buf_.clear();
    return true;
}

bool Process::send(const std::string& line) {
    if (to_child_ < 0) return false;
    std::string s = line + "\n";
    const char* p = s.data();
    std::size_t left = s.size();
    while (left > 0) {
        ssize_t n = write(to_child_, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        p += n;
        left -= std::size_t(n);
    }
    return true;
}

Process::Read Process::read_line(std::string& line, Clock::time_point deadline) {
    for (;;) {
        auto nl = buf_.find('\n');
        if (nl != std::string::npos) {
            line = buf_.substr(0, nl);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            buf_.erase(0, nl + 1);
            return Read::Line;
        }
        if (from_child_ < 0) return Read::Eof;
        auto now = Clock::now();
        if (now >= deadline) return Read::Timeout;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        pollfd pfd{from_child_, POLLIN, 0};
        int r = poll(&pfd, 1, int(std::min<long long>(ms + 1, 1000)));
        if (r < 0) {
            if (errno == EINTR) continue;
            return Read::Eof;
        }
        if (r == 0) continue;
        char tmp[4096];
        ssize_t n = read(from_child_, tmp, sizeof tmp);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return Read::Eof;
        buf_.append(tmp, std::size_t(n));
    }
}

void Process::kill() {
    if (to_child_ >= 0) close(to_child_);
    if (from_child_ >= 0) close(from_child_);
    to_child_ = from_child_ = -1;
    if (pid_ > 0) {
        ::kill(pid_, SIGKILL);
        int status;
        while (waitpid(pid_, &status, 0) < 0 && errno == EINTR) {
        }
    }
    pid_ = -1;
    buf_.clear();
}

bool Engine::expect(const std::string& prefix, std::string& line, double seconds) {
    auto deadline = Clock::now() + std::chrono::duration_cast<Clock::duration>(std::chrono::duration<double>(seconds));
    for (;;) {
        if (proc_.read_line(line, deadline) != Process::Read::Line) return false;
        if (line.rfind(prefix, 0) == 0) return true;
        if (line.rfind("error", 0) == 0) return false;
    }
}

bool Engine::launch(std::string& err) {
    if (!proc_.start(spec_.argv, log_path_, err)) return false;
    std::string line, engine_name;
    auto deadline = Clock::now() + std::chrono::seconds(60);
    for (;;) {
        if (proc_.read_line(line, deadline) != Process::Read::Line) {
            err = spec_.label + ": no 'ready' from " + spec_.argv[0] + " (see " + log_path_ + ")";
            proc_.kill();
            return false;
        }
        if (line.rfind("id name ", 0) == 0) engine_name = line.substr(8);
        else if (line.rfind("error", 0) == 0) {
            err = spec_.label + ": " + line;
            proc_.kill();
            return false;
        } else if (line == "ready") break;
    }
    name_ = spec_.name.empty() ? engine_name : spec_.name;
    for (const auto& c : spec_.cmds) {
        proc_.send("cmd " + c);
        if (!expect("reply", line, 30)) {
            err = spec_.label + ": no reply to 'cmd " + c + "'";
            proc_.kill();
            return false;
        }
        // "reply 0 ...": the engine did not accept the command, so it would play with other settings than asked.
        if (line.rfind("reply 0", 0) == 0) {
            if (!spec_.allow_rejected) {
                err = spec_.label + ": the engine rejected 'cmd " + c + "' (" + line +
                      "); --allow-rejected-options plays on anyway";
                proc_.kill();
                return false;
            }
            std::fprintf(stderr, "warning: %s: the engine rejected 'cmd %s' (%s)\n", spec_.label.c_str(), c.c_str(),
                         line.c_str());
        }
    }
    // Warm-up search so one-time initialisation does not count against the first game's clock.
    proc_.send("newgame");
    if (!expect("ok", line, 30)) {
        err = spec_.label + ": no reply to newgame";
        proc_.kill();
        return false;
    }
    proc_.send("position startpos");
    proc_.send("go maxtime 0.05 reset");
    if (!expect("ok", line, 30) || !expect("bestmove", line, 120)) {
        err = spec_.label + ": warm-up search failed (see " + log_path_ + ")";
        proc_.kill();
        return false;
    }
    return true;
}

bool Engine::new_game() {
    std::string line;
    return proc_.send("newgame") && expect("ok", line, 30);
}

GoResult Engine::go(const std::string& position, const std::string& go_cmd, Clock::time_point stop_at,
                    Clock::time_point kill_at) {
    GoResult r;
    std::string line;
    if (!proc_.send(position)) {
        r.status = GoResult::Status::Crash;
        r.detail = "engine process is gone";
        return r;
    }
    auto ok_deadline = Clock::now() + std::chrono::seconds(30);
    for (;;) {
        auto st = proc_.read_line(line, ok_deadline);
        if (st == Process::Read::Eof) {
            r.status = GoResult::Status::Crash;
            r.detail = "engine exited";
            return r;
        }
        if (st == Process::Read::Timeout) {
            r.status = GoResult::Status::Error;
            r.detail = "no reply to position";
            return r;
        }
        if (line == "ok") break;
        if (line.rfind("error", 0) == 0) {
            r.status = GoResult::Status::Error;
            r.detail = line;
            return r;
        }
    }

    auto t0 = Clock::now();
    if (!proc_.send(go_cmd)) {
        r.status = GoResult::Status::Crash;
        r.detail = "engine process is gone";
        return r;
    }
    bool stopped = false;
    for (;;) {
        auto deadline = stopped ? kill_at : std::min(stop_at, kill_at);
        auto st = proc_.read_line(line, deadline);
        if (st == Process::Read::Timeout) {
            if (!stopped && Clock::now() < kill_at) {
                proc_.send("stop");
                stopped = true;
                continue;
            }
            r.elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
            r.status = GoResult::Status::Timeout;
            r.detail = "no move after " + std::to_string(r.elapsed) + " s";
            proc_.kill();
            return r;
        }
        if (st == Process::Read::Eof) {
            r.elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
            r.status = GoResult::Status::Crash;
            r.detail = "engine exited during search";
            proc_.kill();
            return r;
        }
        if (line.rfind("info ", 0) == 0) {
            r.info = line.substr(5);
        } else if (line.rfind("bestmove ", 0) == 0) {
            r.elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
            std::istringstream in(line.substr(9));
            in >> r.move;
            std::string key, val;
            while (in >> key >> val)
                if (key == "result") r.claim = val;
            r.status = (r.move == "none") ? GoResult::Status::Illegal : GoResult::Status::Ok;
            if (r.move == "none") r.detail = "engine returned no move";
            return r;
        } else if (line.rfind("error", 0) == 0) {
            r.elapsed = std::chrono::duration<double>(Clock::now() - t0).count();
            r.status = line.rfind("error illegal-move", 0) == 0 ? GoResult::Status::Illegal : GoResult::Status::Error;
            r.detail = line;
            return r;
        }
    }
}

}  // namespace match
