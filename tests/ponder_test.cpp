// Pondering in the CheckerBoard engine library: `ponder_test LIB CASE` loads the library and checks one case (hit,
// predict, miss, stop, newgame, teardown, exit, games). Every move getmove returns must be legal.
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <chrono>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>

#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"

using namespace regicide;

namespace {

using Board8x8 = int[8][8];
using GetMove = int (*)(Board8x8, int, double, char*, int*, int, int, void*);
using EngineCommand = int (*)(const char*, char*);
using Clock = std::chrono::steady_clock;

constexpr int CB_RESET_MOVES = 1;

void to_board(const Position& p, Board8x8 b) {
    std::memset(b, 0, sizeof(Board8x8));
    for (int sq = 0; sq < 32; ++sq) {
        const int n = to_pdn(sq) - 1, y = n / 4, x = 2 * (3 - n % 4) + (y & 1);
        if (p.pieces(BLACK) & bit(sq)) b[x][y] = 2 | ((p.kings() & bit(sq)) ? 8 : 4);
        if (p.pieces(WHITE) & bit(sq)) b[x][y] = 1 | ((p.kings() & bit(sq)) ? 8 : 4);
    }
}

double since_ms(Clock::time_point t) { return std::chrono::duration<double, std::milli>(Clock::now() - t).count(); }

void pause_ms(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

struct Lib {
#ifdef _WIN32
    HMODULE h = nullptr;
#else
    void* h = nullptr;
#endif
    GetMove getmove = nullptr;
    EngineCommand command = nullptr;

    bool open(const char* path) {
#ifdef _WIN32
        h = LoadLibraryA(path);
        if (!h) return false;
        getmove = reinterpret_cast<GetMove>(reinterpret_cast<void*>(GetProcAddress(h, "getmove")));
        command = reinterpret_cast<EngineCommand>(reinterpret_cast<void*>(GetProcAddress(h, "enginecommand")));
#else
        h = dlopen(path, RTLD_NOW);
        if (!h) return false;
        getmove = reinterpret_cast<GetMove>(dlsym(h, "getmove"));
        command = reinterpret_cast<EngineCommand>(dlsym(h, "enginecommand"));
#endif
        return getmove && command;
    }
    void close() {
#ifdef _WIN32
        FreeLibrary(h);
#else
        dlclose(h);
#endif
        h = nullptr;
    }

    std::string cmd(const std::string& c) {
        char reply[1024] = "";
        command(c.c_str(), reply);
        return reply;
    }

    // Plays one move in `pos` and returns the position after it (the side to move changes), or false if the board
    // getmove wrote back is not a legal successor.
    struct Played {
        Position after;
        std::string info;
        double ms = 0;
    };
    bool move(const Position& pos, double maxtime, int info, Played& out, int* playnow = nullptr) {
        Board8x8 b;
        to_board(pos, b);
        char str[1024] = "";
        int own_playnow = 0;
        unsigned char cbmove[512];
        const auto t0 = Clock::now();
        getmove(b, pos.side_to_move() == WHITE ? 1 : 2, maxtime, str, playnow ? playnow : &own_playnow, info, 0,
                cbmove);
        out.ms = since_ms(t0);
        out.info = str;
        MoveList list;
        generate_moves(pos, list);
        for (const auto& m : list) {
            Position next = pos;
            next.make_move(m);
            Board8x8 nb;
            to_board(next, nb);
            if (std::memcmp(nb, b, sizeof(Board8x8)) == 0) {
                out.after = next;
                return true;
            }
        }
        std::cerr << "illegal board returned from " << pos.fen() << '\n';
        return false;
    }

    // Waits up to `ms` for the ponder search to start; returns the position it ponders.
    std::optional<Position> ponder_pos(int ms = 2000) {
        for (const auto t0 = Clock::now();; pause_ms(1)) {
            const std::string fen = cmd("get ponderpos");
            Position p;
            if (!fen.empty() && p.set_fen(fen)) return p;
            if (since_ms(t0) >= ms) return std::nullopt;
        }
    }
};

bool check(bool ok, const std::string& what) {
    if (!ok) std::cerr << "FAILED: " << what << '\n';
    return ok;
}

bool contains(const std::string& s, const std::string& what) { return s.find(what) != std::string::npos; }

// A quiet position a few moves into the game (searched with the book off): no captures for a while, so the search
// returns a principal variation and neither side's move is forced.
Position midgame() {
    Position p;
    for (int ply = 0; ply < 8; ++ply) {
        MoveList list;
        generate_moves(p, list);
        for (std::size_t i = 0; i < list.size(); ++i) {
            Position next = p;
            next.make_move(list[(3 * ply + 1 + i) % list.size()]);
            MoveList replies;
            generate_moves(next, replies);
            if (replies.size() > 1 && !replies[0].is_capture()) {
                p = next;
                break;
            }
        }
    }
    return p;
}

// A book position early in the game where, whatever the book plays and the opponent answers, we have a choice.
std::optional<Position> book_position() {
    const auto book = Book::builtin();
    std::vector<Position> level{Position()};
    for (int ply = 0; ply < 6; ++ply) {
        std::vector<Position> next;
        for (const Position& p : level) {
            const auto entries = book->lookup(p);
            bool ok = !entries.empty();
            for (const Book::Entry& e : entries) {
                const auto m = Book::to_move(p, e);
                if (!m) continue;
                Position after = p;
                after.make_move(*m);
                MoveList replies;
                generate_moves(after, replies);
                ok &= replies.size() >= 2;
                for (const auto& r : replies) {
                    Position q = after;
                    q.make_move(r);
                    MoveList ours;
                    generate_moves(q, ours);
                    ok &= ours.size() >= 2;
                }
            }
            if (ok) return p;
            MoveList list;
            generate_moves(p, list);
            for (const auto& m : list) {
                Position q = p;
                q.make_move(m);
                next.push_back(q);
            }
        }
        level = std::move(next);
    }
    return std::nullopt;
}

// A ponder hit after `wait_ms` of pondering: the hit's move is legal, reported as a hit, and in time. With `book`
// our move is a book move, without a reply to expect, so the ponder thread first predicts the reply with a short
// search of its own.
bool test_hit(Lib& lib, bool book, int wait_ms, double maxtime, double max_ms) {
    lib.cmd(std::string("set book ") + (book ? "3" : "0"));
    const auto start = book ? book_position() : midgame();
    if (!check(start.has_value(), "a suitable book position")) return false;
    Lib::Played m;
    if (!lib.move(*start, 0.1, CB_RESET_MOVES, m)) return false;
    if (book && !check(contains(m.info, "depth 0"), "a book move: " + m.info)) return false;
    const auto predicted = lib.ponder_pos();
    if (!check(predicted.has_value(), "ponder search started: " + start->fen() + " " + m.info)) return false;
    MoveList list;
    generate_moves(m.after, list);
    bool follows = false;
    for (const auto& mv : list) {
        Position p = m.after;
        p.make_move(mv);
        follows |= p == *predicted;
    }
    if (!check(follows, "predicted position follows our move")) return false;
    pause_ms(wait_ms);
    Lib::Played h;
    if (!lib.move(*predicted, maxtime, 0, h)) return false;
    std::cout << "hit after " << wait_ms << " ms, maxtime " << maxtime << ": " << h.ms << " ms, " << h.info << '\n';
    return check(contains(h.info, "ponder hit"), "reported as a hit") && check(h.ms < max_ms, "hit answered in time");
}

bool test_miss(Lib& lib) {
    lib.cmd("set book 0");
    Lib::Played m;
    if (!lib.move(midgame(), 0.1, CB_RESET_MOVES, m)) return false;
    const auto predicted = lib.ponder_pos();
    if (!check(predicted.has_value(), "ponder search started")) return false;
    pause_ms(300);
    MoveList list;
    generate_moves(m.after, list);
    Position other;
    for (const auto& mv : list) {
        Position p = m.after;
        p.make_move(mv);
        if (!(p == *predicted)) other = p;
    }
    Lib::Played r;
    if (!lib.move(other, 0.1, 0, r)) return false;
    std::cout << "miss: " << r.ms << " ms, " << r.info << '\n';
    // Hard limit 250 ms (tm_hard) plus slack for a loaded machine.
    return check(contains(r.info, "ponder miss"), "reported as a miss") && check(r.ms < 600, "miss answered in time");
}

// Every library call that changes the engine stops pondering at once; queries and "set gamehist" do not.
bool test_stop(Lib& lib) {
    lib.cmd("set book 0");
    lib.cmd("set threads 2");
    double worst = 0;
    for (int i = 0; i < 10; ++i) {
        Lib::Played m;
        if (!lib.move(midgame(), 0.05, CB_RESET_MOVES, m)) return false;
        if (!check(lib.ponder_pos().has_value(), "ponder search started")) return false;
        pause_ms(50 + 20 * i);
        lib.cmd("get threads");
        lib.cmd("set gamehist " + m.after.fen());
        if (!check(lib.ponder_pos(0).has_value(), "queries leave the ponder search running")) return false;
        const auto t0 = Clock::now();
        lib.cmd("set param tm_drop=0");
        worst = std::max(worst, since_ms(t0));
        if (!check(!lib.ponder_pos(0).has_value(), "a command stops the ponder search")) return false;
    }
    std::cout << "worst stop latency " << worst << " ms\n";
    lib.cmd("set threads 1");
    return check(worst < 50, "stop latency");
}

// A new game (CB_RESET_MOVES) while pondering: a miss, and the new game is played normally.
bool test_newgame(Lib& lib) {
    lib.cmd("set book 0");
    Lib::Played m;
    if (!lib.move(midgame(), 0.1, CB_RESET_MOVES, m)) return false;
    const auto predicted = lib.ponder_pos();
    if (!check(predicted.has_value(), "ponder search started")) return false;
    pause_ms(200);
    // Even the predicted position itself is a new game when CB_RESET_MOVES is set.
    Lib::Played r;
    if (!lib.move(*predicted, 0.1, CB_RESET_MOVES, r)) return false;
    std::cout << "new game: " << r.ms << " ms, " << r.info << '\n';
    return check(contains(r.info, "ponder miss"), "a new game is a miss") && check(r.ms < 600, "answered in time");
}

// Unloading the library (or exiting the process) while pondering stops the thread first.
bool test_teardown(const char* path) {
    for (int round = 0; round < 3; ++round) {
        Lib lib;
        if (!check(lib.open(path), "load library")) return false;
        lib.cmd("set ponder 1");
        lib.cmd("set threads 2");
        lib.cmd("set book 0");
        Lib::Played m;
        if (!lib.move(midgame(), 0.05, CB_RESET_MOVES, m)) return false;
        if (!check(lib.ponder_pos().has_value(), "ponder search started")) return false;
        pause_ms(100);
        const auto t0 = Clock::now();
        lib.close();
        std::cout << "unload while pondering: " << since_ms(t0) << " ms\n";
    }
    // If the system really unloaded the library, it starts afresh (pondering off); otherwise it is still loaded and
    // whatever it was doing has to have survived the dlclose.
    Lib lib;
    if (!check(lib.open(path), "reload library")) return false;
    std::cout << (lib.cmd("get ponder") == "0" ? "the library was unloaded\n" : "the library stayed loaded\n");
    lib.cmd("set ponder 0");
    Lib::Played m;
    return lib.move(midgame(), 0.05, CB_RESET_MOVES, m);
}

// Several games in one process against an opponent that plays the predicted reply half of the time.
bool test_games(Lib& lib) {
    lib.cmd("set book 3");
    std::mt19937 rng(7);
    int hits = 0, misses = 0;
    for (int game = 0; game < 4; ++game) {
        Position pos;
        for (int ply = 0; ply < 120; ply += 2) {
            Lib::Played m;
            if (!lib.move(pos, 0.02, ply == 0 ? CB_RESET_MOVES : 0, m)) return false;
            hits += contains(m.info, "ponder hit");
            misses += contains(m.info, "ponder miss");
            pos = m.after;
            MoveList list;
            generate_moves(pos, list);
            if (list.empty()) break;
            const auto predicted = rng() % 2 ? lib.ponder_pos(200) : std::nullopt;
            if (predicted) {
                pos = *predicted;
            } else {
                pos.make_move(list[rng() % list.size()]);
            }
            MoveList ours;
            generate_moves(pos, ours);
            if (ours.empty()) break;
        }
    }
    std::cout << "games: " << hits << " hits, " << misses << " misses\n";
    return check(hits > 0 && misses > 0, "both hits and misses");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "usage: ponder_test LIB hit|predict|deep|miss|stop|newgame|teardown|exit|games\n";
        return 2;
    }
    const std::string which = argv[2];
    if (which == "teardown") return test_teardown(argv[1]) ? 0 : 1;

    Lib lib;
    if (!lib.open(argv[1])) {
        std::cerr << "cannot load " << argv[1] << '\n';
        return 1;
    }
    if (!check(lib.cmd("get ponder") == "0", "ponder is off by default")) return 1;
    lib.cmd("set ponder 1");
    if (!check(lib.cmd("get ponder") == "1", "set ponder 1")) return 1;

    bool ok = false;
    if (which == "hit") {
        // Long pondering, short move: the ponder search is already deeper and plays at once.
        ok = test_hit(lib, false, 1000, 0.1, 100) &&
             // A hit right away: the search continues as a normal timed search (hard limit 2.5 x 0.4 s).
             test_hit(lib, false, 0, 0.4, 1300);
    } else if (which == "predict") {
        // A book move: the ponder thread predicts the reply with a short search of its own.
        ok = test_hit(lib, true, 500, 0.1, 300);
    } else if (which == "miss") {
        ok = test_miss(lib);
    } else if (which == "stop") {
        ok = test_stop(lib);
    } else if (which == "newgame") {
        ok = test_newgame(lib);
    } else if (which == "exit") {
        // Return from main while pondering: the library's static destructors stop the thread.
        lib.cmd("set book 0");
        lib.cmd("set threads 2");
        Lib::Played m;
        ok = lib.move(midgame(), 0.05, CB_RESET_MOVES, m) && check(lib.ponder_pos().has_value(), "pondering");
        pause_ms(100);
    } else if (which == "games") {
        ok = test_games(lib);
    }
    std::cout << which << (ok ? " passed" : " FAILED") << '\n';
    return ok ? 0 : 1;
}
