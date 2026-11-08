// CheckerBoard engine interface (https://github.com/eygilbert/CheckerBoard, source/cb_interface.h).
// Built as a shared library so Regicide runs inside CheckerBoard on Windows and in our match runner.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
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

}  // namespace

CB_EXPORT int WINAPI getmove(Board8x8 board, int color, double maxtime, char str[1024], int* playnow, int info,
                             int moreinfo, CBmove* move) {
    State& s = state();
    std::lock_guard lock(s.mutex);
    const Position pos = read_board(board, color);

    if (info & CB_RESET_MOVES) s.engine->new_game();
    if (!s.gamehist.empty() && s.gamehist.back() == pos) {
        // CheckerBoard told us the game since the last conversion move: exact, also after takebacks or edits.
        s.history.assign(s.gamehist.begin(), s.gamehist.end() - 1);
    } else if ((info & CB_RESET_MOVES) || !s.have_after || !follows(s.after_our_move, pos)) {
        s.history.clear();
    } else {
        s.history.push_back(s.after_our_move);
    }
    s.gamehist.clear();

    MoveList list;
    generate_moves(pos, list);
    if (list.empty()) {
        if (str) std::snprintf(str, 1024, "no legal moves");
        s.have_after = false;
        return CB_LOSS;
    }

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

    s.engine->set_stop_poll([playnow] { return playnow && *playnow; });
    s.engine->set_position(pos, s.history);
    SearchResult r = s.engine->go(limits, [str](const SearchInfo& i) {
        if (!str) return;
        std::ostringstream o;
        o << "depth " << i.depth << "/" << i.seldepth << "  score " << i.score << "  nodes " << i.nodes << "  "
          << i.nodes / std::max<std::int64_t>(1, i.time_ms) << " kN/s  pv";
        for (std::size_t k = 0; k < i.pv.size() && k < 10; ++k) o << ' ' << move_to_string(i.pv[k]);
        std::snprintf(str, 1024, "%s", o.str().c_str());
    });
    s.engine->set_stop_poll({});

    write_move(board, pos, r.best, move);
    s.history.push_back(pos);
    s.after_our_move = pos;
    s.after_our_move.make_move(r.best);
    s.have_after = true;

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
        if (p1 == "threads") {
            s.threads = std::max(1, std::atoi(p2.c_str()));
            s.engine->set_threads(s.threads);
            return answer(std::to_string(s.threads), 1);
        }
    }
    return answer("?", 0);
}
