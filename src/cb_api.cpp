// CheckerBoard engine interface (https://github.com/eygilbert/CheckerBoard, source/cb_interface.h).
// Built as a shared library so Regicide runs inside CheckerBoard on Windows and in our match runner.
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "egdb/egdb.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "search.hpp"

#if defined(_WIN32)
#include <windows.h>
#define CB_EXPORT extern "C" __declspec(dllexport)
#define WINAPI __stdcall
#else
#include <dlfcn.h>
#define CB_EXPORT extern "C" __attribute__((visibility("default")))
#define WINAPI
#endif

namespace {

using namespace regicide;

constexpr int CB_WHITE = 1, CB_BLACK = 2, CB_MAN = 4, CB_KING = 8;
[[maybe_unused]] constexpr int CB_DRAW = 0;
constexpr int CB_WIN = 1, CB_LOSS = 2, CB_UNKNOWN = 3;
constexpr int CB_RESET_MOVES = 1, CB_EXACT_TIME = 2, CB_INCR_TIME_SHIFT = 2;

struct coor {
    int x, y;
};

struct CBmove {
    int jumps, newpiece, oldpiece;
    coor from, to;
    coor path[12];
    coor del[12];
    int delpiece[12];
};

using Board8x8 = int[8][8];

// CheckerBoard's (x, y) is the standard board rotated 180 degrees: square 1 (b8) is at x = 6, y = 0.
int coors_to_square(int x, int y) { return from_pdn(4 * (y + 1) - x / 2); }
coor square_to_coors(int sq) {
    const int n = to_pdn(sq) - 1;
    const int y = n / 4;
    return {2 * (3 - n % 4) + (y & 1), y};
}

using Clock = std::chrono::steady_clock;

// The ponder thread. On Windows it holds a reference to this library while it runs and drops it as it exits
// (FreeLibraryAndExitThread), so a FreeLibrary during pondering cannot unload the code the thread is running, and
// the library's static destructors never wait for a thread under the loader lock.
class PonderThread {
public:
    bool joinable() const {
#if defined(_WIN32)
        return handle_ != nullptr;
#else
        return thread_.joinable();
#endif
    }
#if defined(_WIN32)
    bool start(std::function<void()> fn) {
        auto* st = new Start{std::move(fn), nullptr};
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCSTR>(&proc), &st->self) ||
            !(handle_ = CreateThread(nullptr, 0, proc, st, 0, nullptr))) {
            if (st->self) FreeLibrary(st->self);
            delete st;
            return false;
        }
        return true;
    }
    void join() {
        WaitForSingleObject(handle_, INFINITE);
        detach();
    }
    void detach() {
        if (handle_) CloseHandle(handle_);
        handle_ = nullptr;
    }
#else
    bool start(std::function<void()> fn) {
        thread_ = std::thread(std::move(fn));
        return true;
    }
    void join() { thread_.join(); }
#endif

private:
#if defined(_WIN32)
    struct Start {
        std::function<void()> fn;
        HMODULE self;
    };
    static DWORD WINAPI proc(void* arg) {
        auto* st = static_cast<Start*>(arg);
        st->fn();
        HMODULE self = st->self;
        delete st;
        FreeLibraryAndExitThread(self, 0);
    }
    HANDLE handle_ = nullptr;
#else
    std::thread thread_;
#endif
};

// Thinking on the opponent's time. After getmove has played, a thread searches the position after the reply the
// search expects (without one, a short search of the position after our move finds it first). The next getmove
// either finds that position (a hit: the running search gets the move's time limits, its clock starting then) or
// stops the thread and searches as usual, on the warm hash table.
struct Ponder {
    PonderThread thread;
    std::atomic<bool> abort{false};
    std::atomic<int*> playnow{nullptr};  // the playnow flag of a getmove waiting for a hit
    Clock::time_point start;             // thread start
    std::int64_t cap_ms = 0;             // the ponder search stops by itself after this long
    // Set by the thread, under `mutex`: just before the ponder search starts, and when it is done.
    std::mutex mutex;
    bool searching = false;
    bool finished = false;
    Position predicted;
    std::vector<Position> history;  // game positions before `predicted`
    // Written by the thread, read after it has been joined.
    SearchResult result;
    std::string info;
};

struct State {
    std::unique_ptr<Engine> engine;
    std::vector<Position> history;  // game positions before the current one
    Position after_our_move;        // position right after our last move, to link the next call to the game
    bool have_after = false;
    std::vector<Position> gamehist;  // from "set gamehist": positions since the last conversion, current last
    std::size_t hash_mb = 128;
    int threads = 1;
    int book_mode = 3;  // CB_BOOK_BEST_MOVES
    std::string egdb_dir;
    std::size_t egdb_cache_mb = 4096;
    std::string evalfile = "builtin";
    bool avoid = true;
    std::string avoidfile = "builtin";
    std::shared_ptr<const Book> avoid_list = Book::builtin();
    bool ponder_on = false;  // "set ponder"
    Ponder ponder;
    std::mutex mutex;
};

// Directory containing this engine library (where an "egdb" folder is looked for by default).
std::string library_dir() {
#if defined(_WIN32)
    HMODULE h = nullptr;
    char path[MAX_PATH] = "";
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       reinterpret_cast<LPCSTR>(&library_dir), &h);
    GetModuleFileNameA(h, path, MAX_PATH);
    std::string p = path;
#else
    Dl_info info{};
    dladdr(reinterpret_cast<void*>(&library_dir), &info);
    std::string p = info.dli_fname ? info.dli_fname : "";
#endif
    const std::size_t slash = p.find_last_of("/\\");
    return slash == std::string::npos ? "." : p.substr(0, slash);
}

void load_egdb(State& s, const std::string& dir, std::size_t cache_mb) {
    const bool first = s.egdb_dir.empty();
    if (egdb::init(dir, cache_mb)) {
        if (first) s.egdb_dir = dir;  // a database already in use stays in use
        s.engine->set_egdb_pieces(egdb::max_pieces());
    }
}

State& state() {
    static State s;
    if (!s.engine) {
        s.engine = std::make_unique<Engine>();
        s.engine->set_hash_mb(s.hash_mb);
        s.engine->set_book(Book::builtin());
        s.engine->set_avoid(s.avoid_list);
        load_egdb(s, library_dir() + "/egdb", s.egdb_cache_mb);
    }
    return s;
}

Position read_board(const Board8x8 b, int color) {
    Bitboard pieces[2] = {0, 0}, kings = 0;
    for (int x = 0; x < 8; ++x)
        for (int y = 0; y < 8; ++y) {
            const int v = b[x][y];
            if (!v || (x + y) % 2) continue;  // playable squares have x + y even
            const int sq = coors_to_square(x, y);
            pieces[(v & CB_WHITE) ? WHITE : BLACK] |= bit(sq);
            if (v & CB_KING) kings |= bit(sq);
        }
    Position p;
    p.set(pieces[BLACK], pieces[WHITE], kings, color == CB_WHITE ? WHITE : BLACK);
    return p;
}

int piece_code(Color c, bool king) { return (c == WHITE ? CB_WHITE : CB_BLACK) | (king ? CB_KING : CB_MAN); }

void write_move(Board8x8 b, const Position& pos, const Move& m, CBmove* out) {
    const Color us = pos.side_to_move();
    const bool king = pos.kings() & bit(m.from);
    const bool crowned = king || (bit(m.to) & promo_row(us));
    const coor f = square_to_coors(m.from), t = square_to_coors(m.to);
    std::vector<int> path = move_path(pos, m);

    if (out) {
        std::memset(out, 0, sizeof(CBmove));
        out->jumps = popcount(m.captures);
        out->oldpiece = piece_code(us, king);
        out->newpiece = piece_code(us, crowned);
        out->from = f;
        out->to = t;
        for (std::size_t i = 1; i + 1 < path.size() && i < 12; ++i) out->path[i] = square_to_coors(path[i]);
        int k = 0;
        for (Bitboard c = m.captures; c && k < 12; ++k) {
            const int sq = pop_lsb(c);
            out->del[k] = square_to_coors(sq);
            out->delpiece[k] = piece_code(~us, pos.kings() & bit(sq));
        }
    }

    b[f.x][f.y] = 0;
    for (Bitboard c = m.captures; c;) {
        const coor d = square_to_coors(pop_lsb(c));
        b[d.x][d.y] = 0;
    }
    b[t.x][t.y] = piece_code(us, crowned);
}

// Does `next` follow from `prev` by one legal move?
bool follows(const Position& prev, const Position& next) {
    MoveList list;
    generate_moves(prev, list);
    for (const Move& m : list) {
        Position p = prev;
        p.make_move(m);
        if (p == next) return true;
    }
    return false;
}

std::string format_info(const SearchInfo& i) {
    std::ostringstream o;
    o << "depth " << i.depth << "/" << i.seldepth << "  score " << i.score << "  nodes " << i.nodes << "  "
      << i.nodes / std::max<std::int64_t>(1, i.time_ms) << " kN/s  pv";
    for (std::size_t k = 0; k < i.pv.size() && k < 10; ++k) o << ' ' << move_to_string(i.pv[k]);
    return o.str();
}

std::int64_t since_ms(Clock::time_point t) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - t).count();
}

// The ponder thread. `reply` is the expected reply to our move, if the search gave one.
void ponder_main(State& s, Position after, std::vector<Position> history, std::optional<Move> reply,
                 int predict_ms) {
    Ponder& p = s.ponder;
    if (!reply) {
        Limits l;
        l.movetime_ms = predict_ms;
        s.engine->set_position(after, history);
        const SearchResult r = s.engine->go(l);
        if (!r.pv.empty()) reply = r.best;
    }
    history.push_back(after);
    Position root = after;
    if (reply) root.make_move(*reply);
    MoveList list;
    if (reply) generate_moves(root, list);
    // Nothing to think about if the game ends there or our move is forced.
    if (list.size() >= 2 && !p.abort.load()) {
        s.engine->set_position(root, history);
        {
            std::lock_guard lock(p.mutex);
            s.engine->expect_ponder();
            p.searching = true;
            p.predicted = root;
            p.history = history;
        }
        Limits l;
        l.ponder = true;
        p.result = s.engine->go(l, [&p](const SearchInfo& i) { p.info = format_info(i); });
    }
    std::lock_guard lock(p.mutex);
    p.finished = true;
}

// Stops the ponder thread (abort and join) and clears its state.
void stop_ponder(State& s) {
    Ponder& p = s.ponder;
    if (!p.thread.joinable()) return;
    p.abort = true;
    s.engine->stop();
    p.thread.join();
    s.engine->set_stop_poll({});
    p.abort = false;
    std::lock_guard lock(p.mutex);
    p.searching = false;
}

// Stops pondering when the library is unloaded or the process exits. Constructed (at the first ponder) after the
// statics the search uses, such as the endgame database's, so it is destroyed before them.
struct PonderReaper {
    ~PonderReaper() {
#if defined(_WIN32)
        // The thread holds a reference to the library, so either this runs on the thread itself as it drops the
        // last one, or the thread is already inside FreeLibraryAndExitThread, or the process is exiting and Windows
        // has ended it: there is nothing to wait for, and waiting under the loader lock would deadlock.
        state().ponder.thread.detach();
#else
        stop_ponder(state());
#endif
    }
};

// Thinks about the position after our move `best` (now s.after_our_move) until the next library call.
void start_ponder(State& s, const SearchResult& r, const Limits& limits) {
    static PonderReaper reaper;
    Ponder& p = s.ponder;
    const Position& after = s.after_our_move;
    MoveList replies;
    generate_moves(after, replies);
    if (replies.empty() || is_decisive(r.score)) return;  // the game is decided

    std::optional<Move> reply;
    if (replies.size() == 1) reply = replies[0];
    if (r.pv.size() >= 2 && r.pv[0] == r.best && std::ranges::find(replies, r.pv[1]) != replies.end())
        reply = r.pv[1];
    // The time this move was given; a reply is predicted with a quarter of it, and the ponder search stops by
    // itself after ten times it (a game abandoned while pondering does not keep the CPU busy).
    const std::int64_t budget = limits.movetime_ms   ? limits.movetime_ms
                                : limits.target_ms ? limits.target_ms
                                                   : limits.time_ms / 32 + limits.inc_ms;
    const int predict_ms = int(std::clamp<std::int64_t>(budget / 4, 10, 1000));
    p.cap_ms = std::max<std::int64_t>(1000, budget * 10);
    p.start = Clock::now();
    p.searching = p.finished = false;
    p.info.clear();
    s.engine->set_stop_poll([&s] {
        Ponder& p = s.ponder;
        if (p.abort.load(std::memory_order_relaxed)) return true;
        if (const int* playnow = p.playnow.load(std::memory_order_acquire);
            playnow && *static_cast<const volatile int*>(playnow))
            return true;
        return s.engine->pondering() && since_ms(p.start) > p.cap_ms;
    });
    if (!p.thread.start([&s, after, history = s.history, reply, predict_ms]() mutable {
            ponder_main(s, after, std::move(history), reply, predict_ms);
        }))
        s.engine->set_stop_poll({});
}

}  // namespace

CB_EXPORT int WINAPI getmove(Board8x8 board, int color, double maxtime, char str[1024], int* playnow, int info,
                             int moreinfo, CBmove* move) {
    State& s = state();
    std::lock_guard lock(s.mutex);
    const Position pos = read_board(board, color);
    const bool reset = info & CB_RESET_MOVES;

    if (!s.gamehist.empty() && s.gamehist.back() == pos) {
        // CheckerBoard told us the game since the last conversion move: exact, also after takebacks or edits.
        s.history.assign(s.gamehist.begin(), s.gamehist.end() - 1);
    } else if (reset || !s.have_after || !follows(s.after_our_move, pos)) {
        s.history.clear();
    } else {
        s.history.push_back(s.after_our_move);
    }
    s.gamehist.clear();

    Limits limits;
    const int tcmode = (info >> CB_INCR_TIME_SHIFT) & 3;
    if (tcmode) {
        const double mult[] = {0.001, 0.01, 0.1};
        limits.inc_ms = int(std::uint16_t(moreinfo & 0xFFFF) * mult[tcmode - 1] * 1000);
        limits.time_ms = std::max(1, int(std::uint16_t(std::uint32_t(moreinfo) >> 16) * mult[tcmode - 1] * 1000));
    } else if (info & CB_EXACT_TIME) {
        limits.movetime_ms = std::max(1, int(maxtime * 1000));
    } else {
        limits.target_ms = std::max(1, int(maxtime * 1000));
    }

    // A ponder hit is the predicted position in the same game, with the same history (CheckerBoard's history starts
    // at the last conversion, so it may be a suffix of ours).
    std::optional<SearchResult> pondered;
    std::string ponder_note;
    if (s.ponder.thread.joinable()) {
        Ponder& p = s.ponder;
        bool hit, idle;
        {
            std::lock_guard plock(p.mutex);
            hit = !reset && p.searching && p.predicted == pos && p.history.size() >= s.history.size() &&
                  std::equal(s.history.begin(), s.history.end(), p.history.end() - s.history.size());
            idle = p.finished && !p.searching;  // there was nothing to ponder (our next move was forced)
        }
        const std::int64_t ms = since_ms(p.start);
        if (idle) {
            stop_ponder(s);
        } else if (hit) {
            p.playnow.store(playnow, std::memory_order_release);
            s.engine->ponderhit(limits);
            p.thread.join();
            p.playnow = nullptr;
            s.engine->set_stop_poll({});
            pondered = p.result;
            if (str) std::snprintf(str, 1024, "%s", p.info.c_str());
            std::lock_guard plock(p.mutex);
            p.searching = false;
        } else {
            stop_ponder(s);
        }
        if (!idle) ponder_note = std::string(hit ? "ponder hit " : "ponder miss ") + std::to_string(ms) + " ms";
    }
    if (reset) s.engine->new_game();

    MoveList list;
    generate_moves(pos, list);
    if (list.empty()) {
        if (str) std::snprintf(str, 1024, "no legal moves");
        s.have_after = false;
        return CB_LOSS;
    }

    SearchResult r;
    if (pondered) {
        r = *pondered;
    } else {
        s.engine->set_stop_poll([playnow] { return playnow && *playnow; });
        s.engine->set_position(pos, s.history);
        r = s.engine->go(limits, [str](const SearchInfo& i) {
            if (str) std::snprintf(str, 1024, "%s", format_info(i).c_str());
        });
        s.engine->set_stop_poll({});
    }
    // For match logs: whether the move came from the ponder search, and how long it had been thinking.
    if (str && !ponder_note.empty()) {
        const std::string text = std::string(str) + (str[0] ? "  " : "") + ponder_note;
        std::snprintf(str, 1024, "%s", text.c_str());
    }

    write_move(board, pos, r.best, move);
    s.history.push_back(pos);
    s.after_our_move = pos;
    s.after_our_move.make_move(r.best);
    s.have_after = true;
    if (s.ponder_on) start_ponder(s, r, limits);

    if (r.score >= SCORE_KNOWN_WIN) return CB_WIN;
    if (r.score <= -SCORE_KNOWN_WIN) return CB_LOSS;
    return CB_UNKNOWN;
}

CB_EXPORT int WINAPI enginecommand(const char* command, char reply[1024]) {
    State& s = state();
    std::istringstream in(command ? command : "");
    std::string cmd, p1, p2;
    in >> cmd >> p1 >> p2;
    auto answer = [&](const std::string& text, int ret) {
        std::snprintf(reply, 1024, "%s", text.c_str());
        return ret;
    };

    // Queries and the game history leave a ponder search running; anything else stops it first.
    const bool keeps_ponder = cmd == "name" || cmd == "about" || cmd == "help" || cmd == "get" ||
                              (cmd == "set" && p1 == "gamehist");
    if (!keeps_ponder) {
        std::lock_guard lock(s.mutex);
        stop_ponder(s);
    }

    if (cmd == "name") return answer("Regicide 0.2", 1);
    if (cmd == "about") return answer("Regicide 0.2\nAn English checkers engine.", 1);
    if (cmd == "help") return answer("", 0);
    if (cmd == "get") {
        if (p1 == "protocolversion") return answer("2", 1);
        if (p1 == "gametype") return answer("21", 1);
        if (p1 == "hashsize") return answer(std::to_string(s.hash_mb), 1);
        if (p1 == "threads") return answer(std::to_string(s.threads), 1);
        if (p1 == "book") return answer(std::to_string(s.book_mode), 1);
        if (p1 == "dbpath") return answer(s.egdb_dir, 1);
        if (p1 == "dbpieces") return answer(std::to_string(s.engine->egdb_pieces()), 1);
        if (p1 == "evalfile") return answer(s.evalfile, 1);
        if (p1 == "avoid") return answer(s.avoid ? "1" : "0", 1);
        if (p1 == "avoidfile") return answer(s.avoidfile, 1);
        if (p1 == "ponder") return answer(s.ponder_on ? "1" : "0", 1);
        if (p1 == "ponderpos") {
            // The position the running ponder search is thinking about (FEN), or "" (for tests).
            std::lock_guard plock(s.ponder.mutex);
            return answer(s.ponder.searching ? s.ponder.predicted.fen() : "", 1);
        }
    }
    if (cmd == "set") {
        std::lock_guard lock(s.mutex);
        if (p1 == "hashsize") {
            s.hash_mb = std::max(1, std::atoi(p2.c_str()));
            s.engine->set_hash_mb(s.hash_mb);
            return answer(std::to_string(s.hash_mb), 1);
        }
        if (p1 == "dbpath") {
            // Everything after "set dbpath " (paths may contain spaces).
            const std::string full = command;
            const std::size_t at = full.find("dbpath");
            load_egdb(s, full.substr(at + 7), s.egdb_cache_mb);
            return answer(std::to_string(s.engine->egdb_pieces()), 1);
        }
        if (p1 == "evalfile") {
            // "set evalfile <path|builtin|hce>": linear weights (RGEV) or an NNUE net (RGNN); paths may contain spaces.
            const std::string full = command;
            const std::string path = full.substr(full.find("evalfile") + 9);
            std::string error;
            auto ev = path == "builtin" ? Evaluator::builtin()
                      : path == "hce"   ? Evaluator::hce()
                                        : Evaluator::from_file(path, &error);
            if (!ev) return answer(error, 0);
            s.engine->set_evaluator(ev);
            s.evalfile = path;
            return answer(ev->network() ? "nnue" : ev->trained() ? "linear" : "hce", 1);
        }
        if (p1 == "avoid") {
            // "set avoid 0|1": turn the avoid list (root moves never played) off or on.
            s.avoid = std::atoi(p2.c_str()) != 0;
            s.engine->set_avoid(s.avoid ? s.avoid_list : nullptr);
            return answer(s.avoid ? "1" : "0", 1);
        }
        if (p1 == "avoidfile") {
            // "set avoidfile <path|builtin>": the avoid list of a book file (RGBK version 2); paths may contain spaces.
            const std::string full = command;
            const std::string path = full.substr(full.find("avoidfile") + 10);
            if (path == "builtin") {
                s.avoid_list = Book::builtin();
            } else {
                auto b = std::make_shared<Book>();
                if (!b->load(path)) return answer("cannot load " + path, 0);
                s.avoid_list = b;
            }
            s.avoidfile = path;
            s.engine->set_avoid(s.avoid ? s.avoid_list : nullptr);
            return answer(std::to_string(s.avoid_list->avoid_size()), 1);
        }
        if (p1 == "gamehist") {
            // "set gamehist <FEN> <m1> <m2> ...": the position after the last conversion and the moves since.
            s.gamehist.clear();
            Position p;
            if (!p.set_fen(p2)) return answer("?", 0);
            s.gamehist.push_back(p);
            for (std::string tok; in >> tok;) {
                Move m;
                if (!parse_move(p, tok, m)) {
                    s.gamehist.clear();
                    return answer("?", 0);
                }
                p.make_move(m);
                s.gamehist.push_back(p);
            }
            return answer("", 1);
        }
        if (p1 == "param") {
            // "set param name=value[,name=value...]": search parameters (see SearchParams), e.g. for tuning matches.
            if (!s.engine->params.set_all(p2)) return answer("?", 0);
            return answer(p2, 1);
        }
        if (p1 == "dbmbytes") {
            // The database is already mapped at startup; a larger budget pre-faults more of it now.
            s.egdb_cache_mb = std::size_t(std::max(0, std::atoi(p2.c_str())));
            egdb::set_cache(s.egdb_cache_mb);
            return answer(std::to_string(s.egdb_cache_mb), 1);
        }
        if (p1 == "book") {
            // 0 = off, 1 = any book move, 2 = good moves (within 0.1 man), 3 = best moves only.
            s.book_mode = std::clamp(std::atoi(p2.c_str()), 0, 3);
            const int margin[] = {0, 60, 10, 0};
            s.engine->set_book(s.book_mode ? Book::builtin() : nullptr, margin[s.book_mode]);
            return answer(std::to_string(s.book_mode), 1);
        }
        if (p1 == "ponder") {
            // "set ponder 0|1": think on the opponent's time, with the configured threads (default off).
            s.ponder_on = std::atoi(p2.c_str()) != 0;
            return answer(s.ponder_on ? "1" : "0", 1);
        }
        if (p1 == "threads") {
            s.threads = std::max(1, std::atoi(p2.c_str()));
            s.engine->set_threads(s.threads);
            return answer(std::to_string(s.threads), 1);
        }
    }
    return answer("?", 0);
}
