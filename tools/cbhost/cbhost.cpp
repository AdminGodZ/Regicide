// cbhost: load a CheckerBoard engine (.dylib/.so exporting getmove/enginecommand) and drive it
// over a line protocol on stdin/stdout. One process per engine instance, because CheckerBoard
// engines keep global state and are not reentrant. See PROTOCOL.md.
// Builds on macOS/Linux (dlopen) and Windows (LoadLibrary; 64-bit CheckerBoard DLLs).

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "cb_api.h"
#include "checkers.hpp"

namespace {

using namespace cbt;

FILE* g_out = nullptr;  // protocol channel (the original stdout)
std::mutex g_out_mutex;

void send(const std::string& line) {
    std::lock_guard lock(g_out_mutex);
    std::fputs(line.c_str(), g_out);
    std::fputc('\n', g_out);
    std::fflush(g_out);
}

std::string escape(const char* s) {
    std::string r;
    for (; *s; ++s) {
        if (*s == '\n') r += "\\n";
        else if (*s == '\r') continue;
        else if (*s == '\\') r += "\\\\";
        else r += *s;
    }
    return r;
}

// Engine output goes into fixed C buffers; keep generous slack in case an engine overruns.
struct alignas(16) ReplyBuffer {
    char text[4096];
};

struct Engine {
    CB_GETMOVE getmove = nullptr;
    CB_ENGINECOMMAND enginecommand = nullptr;

    bool command(const std::string& cmd, std::string& reply) {
        if (!enginecommand) return false;
        std::vector<char> buf(std::max<std::size_t>(cmd.size() + 1, 1024), 0);
        std::memcpy(buf.data(), cmd.c_str(), cmd.size() + 1);
        ReplyBuffer rb{};
        int ok = enginecommand(buf.data(), rb.text);
        rb.text[sizeof(rb.text) - 1] = 0;
        reply = rb.text;
        return ok != 0;
    }
};

volatile int g_playnow = 0;
std::atomic<bool> g_searching{false};
// A `stop` belongs to the `go` read last. It can arrive before that go reaches the engine (the command is still
// queued, or the game history is being sent), so it is remembered by the go's number and honoured on entry. Several
// queued gos can each have their own stop.
std::atomic<long> g_go_read{0};       // `go` lines read so far
std::atomic<long> g_searching_go{0};  // number of the go inside the engine (0: none)
std::mutex g_stop_mutex;
std::set<long> g_stopped;             // gos a `stop` was meant for that have not been entered yet
std::atomic<bool> g_shutdown{false};  // `quit` or end of input was read: every go still to come starts stopped

std::mutex g_queue_mutex;
std::condition_variable g_queue_cv;
std::deque<std::string> g_queue;

void reader_thread() {
    std::string line;
    while (std::getline(std::cin, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
        if (line == "stop") {
            const long target = g_go_read.load();
            {
                std::lock_guard lock(g_stop_mutex);
                g_stopped.insert(target);
            }
            if (g_searching_go == target) g_playnow = 1;
            continue;
        }
        if (line == "quit") {  // also ends the search of every go that is still queued or being prepared
            g_shutdown = true;
            if (g_searching) g_playnow = 1;
        }
        std::istringstream words(line);
        std::string first;
        if (words >> first && first == "go") ++g_go_read;
        std::lock_guard lock(g_queue_mutex);
        g_queue.push_back(line);
        g_queue_cv.notify_one();
    }
    g_shutdown = true;  // end of input: as `quit`
    g_playnow = 1;
    std::lock_guard lock(g_queue_mutex);
    g_queue.push_back("quit");
    g_queue_cv.notify_one();
}

std::string next_command() {
    std::unique_lock lock(g_queue_mutex);
    g_queue_cv.wait(lock, [] { return !g_queue.empty(); });
    std::string s = std::move(g_queue.front());
    g_queue.pop_front();
    return s;
}

struct Game {
    Board start = Board::start();  // position given to `position`
    std::vector<Move> moves;       // moves played since `start`
    Board current = Board::start();

    // CheckerBoard's send_game_history(): FEN at the last conversion move followed by the
    // reversible (king, non-capture) moves after it, as from-to pairs.
    std::string gamehist() const {
        std::vector<Board> boards{start};
        for (const Move& m : moves) boards.push_back(boards.back().after(m));
        std::size_t first = moves.size();
        while (first > 0 && !boards[first - 1].is_conversion(moves[first - 1])) --first;
        std::string s = "set gamehist " + boards[first].fen();
        for (std::size_t i = first; i < moves.size(); ++i)
            s += " " + std::to_string(moves[i].from + 1) + "-" + std::to_string(moves[i].to + 1);
        return s;
    }
};

const char* result_name(int r) {
    switch (r) {
        case CB_DRAW: return "draw";
        case CB_WIN: return "win";
        case CB_LOSS: return "loss";
        default: return "unknown";
    }
}

// Work out which legal move the engine played: compare the board it returned against every
// legal successor (what CheckerBoard does), falling back to the CBmove struct for engines that
// leave the board untouched.
std::optional<Move> identify_move(const Board& before, const int board8[8][8], const CBmove& cbm, std::string& why) {
    auto moves = before.legal_moves();
    Board returned = Board::from_board8(board8, opposite(before.stm));
    for (const Move& m : moves)
        if (before.after(m) == returned) return m;

    Board unchanged = before;
    unchanged.stm = opposite(before.stm);
    if (!(returned == unchanged)) {
        why = "returned board matches no legal move: " + returned.fen();
        return std::nullopt;
    }
    int from = xy_square(cbm.from.x, cbm.from.y), to = xy_square(cbm.to.x, cbm.to.y);
    std::uint32_t caps = 0;
    for (int i = 0; i < cbm.jumps && i < 12; ++i) {
        int sq = xy_square(cbm.del[i].x, cbm.del[i].y);
        if (sq >= 0) caps |= bit(sq);
    }
    for (const Move& m : moves)
        if (m.from == from && m.to == to && m.captures == caps) return m;
    why = "board unchanged and CBmove (" + std::to_string(from + 1) + "-" + std::to_string(to + 1) +
          ", " + std::to_string(cbm.jumps) + " jumps) is not a legal move";
    return std::nullopt;
}

int run(Engine& eng, bool send_hist) {
    Game game;
    bool reset_pending = true;
    long gos_handled = 0;  // counts `go` commands as the reader does (g_go_read)

    for (;;) {
        std::string line = next_command();
        std::istringstream in(line);
        std::string cmd;
        in >> cmd;
        if (cmd.empty()) continue;

        if (cmd == "quit") return 0;
        if (cmd == "ping") {
            send("pong");
        } else if (cmd == "name") {
            std::string reply;
            send("id name " + (eng.command("name", reply) ? escape(reply.c_str()) : std::string("unknown")));
        } else if (cmd == "cmd") {
            std::string rest;
            std::getline(in >> std::ws, rest);
            std::string reply;
            bool ok = eng.command(rest, reply);
            send(std::string("reply ") + (ok ? "1 " : "0 ") + escape(reply.c_str()));
        } else if (cmd == "newgame") {
            reset_pending = true;
            game = Game{};
            send("ok");
        } else if (cmd == "position") {
            std::string kind;
            in >> kind;
            std::optional<Board> b;
            std::string err, tok;
            if (kind == "startpos") {
                b = Board::start();
            } else if (kind == "fen") {
                std::string fen;
                in >> fen;
                b = Board::from_fen(fen, &err);
            } else {
                err = "position: expected startpos or fen";
            }
            Game g;
            if (b) {
                g.start = g.current = *b;
                if (in >> tok && tok == "moves") {
                    std::string rest;
                    std::getline(in, rest);
                    auto after = play_moves(*b, rest, &g.moves, &err);
                    if (after) g.current = *after;
                    else b.reset();
                } else if (!tok.empty()) {
                    err = "position: unexpected '" + tok + "'";
                    b.reset();
                }
            }
            if (b) {
                game = std::move(g);
                send("ok");
            } else {
                send("error " + err);
            }
        } else if (cmd == "go") {
            const long go_number = ++gos_handled;
            double maxtime = 1.0, inc = -1, remaining = -1;
            int info = 0, moreinfo = 0;
            bool raw_info = false;
            std::string tok;
            while (in >> tok) {
                if (tok == "maxtime") in >> maxtime;
                else if (tok == "exact") info |= CB_EXACT_TIME;
                else if (tok == "reset") reset_pending = true;
                else if (tok == "inc") in >> inc;
                else if (tok == "remaining") in >> remaining;
                else if (tok == "info") { in >> info; raw_info = true; }
                else if (tok == "moreinfo") { in >> moreinfo; raw_info = true; }
            }
            if (!raw_info && inc >= 0 && remaining >= 0) cb_format_time_args(inc, remaining, &info, &moreinfo);
            if (reset_pending) info |= CB_RESET_MOVES;

            const Board before = game.current;
            if (before.legal_moves().empty()) {
                send("bestmove none result loss time 0");
                continue;
            }
            if (send_hist && eng.enginecommand) {
                std::string reply;
                eng.command(game.gamehist(), reply);
            }

            int board8[8][8];
            before.to_board8(board8);
            CBmove cbm{};
            char str[4096] = {0};  // CB promises 1024; extra slack for sloppy engines
            g_playnow = 0;
            g_searching_go = go_number;
            g_searching = true;
            bool stopped = false;  // while this go was queued or being prepared
            {
                std::lock_guard lock(g_stop_mutex);
                stopped = g_stopped.contains(go_number);
                g_stopped.erase(g_stopped.begin(), g_stopped.upper_bound(go_number));
            }
            if (stopped || g_shutdown) g_playnow = 1;
            auto t0 = std::chrono::steady_clock::now();
            int r = eng.getmove(board8, before.stm == Color::Black ? CB_BLACK : CB_WHITE, maxtime, str,
                                const_cast<int*>(&g_playnow), info, moreinfo, &cbm);
            double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            g_searching = false;
            g_searching_go = 0;
            g_playnow = 0;
            reset_pending = false;
            str[sizeof(str) - 1] = 0;

            send("info " + escape(str));
            std::string why;
            auto m = identify_move(before, board8, cbm, why);
            if (!m) {
                send("error illegal-move " + why);
                continue;
            }
            game.moves.push_back(*m);
            game.current = before.after(*m);
            char tbuf[64];
            std::snprintf(tbuf, sizeof tbuf, "%.4f", elapsed);
            send("bestmove " + move_to_pdn(*m) + " result " + result_name(r) + " time " + tbuf);
        } else {
            send("error unknown command '" + cmd + "'");
        }
    }
}

void usage() {
    std::fprintf(stderr,
                 "usage: cbhost [--cwd DIR] [--no-gamehist] ENGINE.dylib\n"
                 "  Loads a CheckerBoard-API engine and speaks the cbhost line protocol on stdin/stdout.\n"
                 "  The working directory defaults to the engine's directory (engines find data files there).\n");
}

}  // namespace

int main(int argc, char** argv) {
    std::string path, cwd;
    bool send_hist = true;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--cwd" && i + 1 < argc) cwd = argv[++i];
        else if (a == "--no-gamehist") send_hist = false;
        else if (a == "-h" || a == "--help") { usage(); return 0; }
        else path = a;
    }
    if (path.empty()) {
        usage();
        return 2;
    }

    // Keep the real stdout for the protocol; anything the engine prints goes to stderr.
#ifdef _WIN32
    int proto_fd = _dup(1);
    _dup2(2, 1);
    SetStdHandle(STD_OUTPUT_HANDLE, GetStdHandle(STD_ERROR_HANDLE));  // engines using WriteFile
    g_out = _fdopen(proto_fd, "wb");                                  // "\n" line ends, no CRLF
    _setmode(_fileno(stdin), _O_BINARY);
#else
    int proto_fd = dup(STDOUT_FILENO);
    dup2(STDERR_FILENO, STDOUT_FILENO);
    g_out = fdopen(proto_fd, "w");
#endif

    namespace fs = std::filesystem;
    fs::path abs = fs::absolute(path);
    std::error_code ec;
    fs::current_path(cwd.empty() ? abs.parent_path() : fs::path(cwd), ec);
    if (ec) {
        send("error cannot chdir to " + (cwd.empty() ? abs.parent_path().string() : cwd));
        return 1;
    }

    Engine eng;
#ifdef _WIN32
    // Resolve the engine's own dependencies (e.g. KingsRow's egdb64.dll) from its directory too.
    HMODULE lib = LoadLibraryExW(abs.wstring().c_str(), nullptr,
                                 LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!lib) {
        send("error LoadLibrary failed for " + abs.string() + " (Windows error " + std::to_string(GetLastError()) +
             "; a 64-bit engine DLL is required)");
        return 1;
    }
    eng.getmove = reinterpret_cast<CB_GETMOVE>(reinterpret_cast<void*>(GetProcAddress(lib, "getmove")));
    eng.enginecommand =
        reinterpret_cast<CB_ENGINECOMMAND>(reinterpret_cast<void*>(GetProcAddress(lib, "enginecommand")));
#else
    void* lib = dlopen(abs.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!lib) {
        send(std::string("error dlopen: ") + dlerror());
        return 1;
    }
    eng.getmove = reinterpret_cast<CB_GETMOVE>(dlsym(lib, "getmove"));
    eng.enginecommand = reinterpret_cast<CB_ENGINECOMMAND>(dlsym(lib, "enginecommand"));
#endif
    if (!eng.getmove) {
        send("error engine does not export getmove");
        return 1;
    }

    std::string name;
    if (!eng.command("name", name)) name = abs.stem().string();
    send("id name " + escape(name.c_str()));
    send("id protocol cbhost 1");
    send("ready");

    std::thread(reader_thread).detach();
    int rc = run(eng, send_hist);
    std::fflush(g_out);
    std::fflush(stdout);
    // Engines may have threads/static destructors that misbehave at unload; skip them.
    std::_Exit(rc);
}
