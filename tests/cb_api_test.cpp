// Loads the CheckerBoard engine library and plays a self-play game through getmove(), checking that every
// returned board is reachable from the previous one by exactly one legal move.
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstring>
#include <filesystem>
#include <iostream>

#include "book.hpp"
#include "movegen.hpp"
#include "position.hpp"
#include "test_egdb.hpp"

using namespace regicide;

namespace {

using Board8x8 = int[8][8];
using GetMove = int (*)(Board8x8, int, double, char*, int*, int, int, void*);
using EngineCommand = int (*)(const char*, char*);

void to_board(const Position& p, Board8x8 b) {
    std::memset(b, 0, sizeof(Board8x8));
    for (int sq = 0; sq < 32; ++sq) {
        const int n = to_pdn(sq) - 1, y = n / 4, x = 2 * (3 - n % 4) + (y & 1);
        if (p.pieces(BLACK) & bit(sq)) b[x][y] = 2 | ((p.kings() & bit(sq)) ? 8 : 4);
        if (p.pieces(WHITE) & bit(sq)) b[x][y] = 1 | ((p.kings() & bit(sq)) ? 8 : 4);
    }
}

}  // namespace

int main(int argc, char** argv) {
    const char* lib = argc > 1 ? argv[1] : "libregicide_cb.dylib";
#ifdef _WIN32
    HMODULE h = LoadLibraryA(lib);
    if (!h) {
        std::cerr << "LoadLibrary failed: error " << GetLastError() << '\n';
        return 1;
    }
    auto getmove = reinterpret_cast<GetMove>(reinterpret_cast<void*>(GetProcAddress(h, "getmove")));
    auto command = reinterpret_cast<EngineCommand>(reinterpret_cast<void*>(GetProcAddress(h, "enginecommand")));
#else
    void* h = dlopen(lib, RTLD_NOW);
    if (!h) {
        std::cerr << "dlopen failed: " << dlerror() << '\n';
        return 1;
    }
    auto getmove = reinterpret_cast<GetMove>(dlsym(h, "getmove"));
    auto command = reinterpret_cast<EngineCommand>(dlsym(h, "enginecommand"));
#endif
    if (!getmove || !command) return 1;

    char reply[1024];
    command("name", reply);
    std::cout << "engine: " << reply << '\n';

    // Game history as CheckerBoard sends it: FEN after the last conversion, then reversible king moves.
    if (command("set gamehist W:WK18:BK3 18-15 3-7 15-18 7-3", reply) != 1 ||
        command("set gamehist W:WK18:BK3 18-11", reply) != 0) {
        std::cerr << "set gamehist failed\n";
        return 1;
    }

    // Evaluation files: RGEV or RGNN by header; a missing file is refused and keeps the current eval.
    if (command("set evalfile " REGICIDE_SOURCE_DIR "/src/weights/eval.bin", reply) != 1 ||
        std::string(reply) != "linear" || command("set evalfile /nonexistent/net.bin", reply) != 0 ||
        command("set evalfile builtin", reply) != 1) {
        std::cerr << "set evalfile failed: " << reply << '\n';
        return 1;
    }

    // Avoid list: with every opening move but one avoided, that one is played; "set avoid 0" turns it off.
    {
        const Position start;
        MoveList list;
        generate_moves(start, list);
        Book avoid;
        for (std::size_t i = 0; i < list.size(); ++i)
            if (i != 3) avoid.add_avoid(Book::make_entry(start, list[i], 0, 1));
        avoid.finalize();
        const std::string path = (std::filesystem::temp_directory_path() / "regicide_cb_avoid.bin").string();
        avoid.save(path);
        Position expected = start;
        expected.make_move(list[3]);
        Board8x8 want, b;
        to_board(expected, want);
        to_board(start, b);
        char str[1024] = "";
        int playnow = 0;
        unsigned char cbmove[512];
        const bool loaded = command(("set avoidfile " + path).c_str(), reply) == 1 &&
                            std::string(reply) == std::to_string(list.size() - 1);
        getmove(b, 2, 0.01, str, &playnow, 1, 0, cbmove);
        std::filesystem::remove(path);
        if (!loaded || std::memcmp(b, want, sizeof(Board8x8)) != 0 || command("set avoid 0", reply) != 1 ||
            command("get avoid", reply) != 1 || std::string(reply) != "0" || command("set avoid 1", reply) != 1 ||
            command("set avoidfile builtin", reply) != 1 || command("set avoidfile /nonexistent.bin", reply) != 0) {
            std::cerr << "avoid list failed: " << reply << '\n';
            return 1;
        }
    }

    // Search parameters: known names are accepted (and set back to their defaults here), unknown ones rejected.
    if (command("set param tm_drop=40,tm_drop_factor=200", reply) != 1 || command("set param tm_drop=0", reply) != 1 ||
        command("set param no_such_param=1", reply) != 0) {
        std::cerr << "set param failed: " << reply << '\n';
        return 1;
    }

    // Malformed commands are answered as not handled; none may throw out of the library.
    for (const char* bad : {"set evalfile", "set avoidfile", "set dbpath", "set param tm_soft=abc", "set param tm_soft",
                            "set", ""})
        if (command(bad, reply) != 0) {
            std::cerr << "malformed command accepted: '" << bad << "' -> " << reply << '\n';
            return 1;
        }

    // A search stopped before it has scored a move knows nothing: no win or loss claim (3 = unknown).
    {
        if (command("get book", reply) != 1) return 1;
        const std::string book = reply;
        command("set book 0", reply);
        Board8x8 b;
        to_board(Position(), b);
        char str[1024] = "";
        int playnow = 1;
        unsigned char cbmove[512];
        const int claim = getmove(b, 2, 0.01, str, &playnow, 1, 0, cbmove);
        if (claim != 3) {
            std::cerr << "a search stopped at once claims " << claim << '\n';
            return 1;
        }
        command(("set book " + book).c_str(), reply);
    }

    // A known loss is not conceded (CheckerBoard's early adjudication would end the game at once): B:W10:B2 loses
    // the last black man to 10x1 or 10x3, and getmove claims nothing; without a legal move it returns the loss.
    {
        auto claim_from = [&](const char* fen) {
            Position p;
            p.set_fen(fen);
            Board8x8 b;
            to_board(p, b);
            char str[1024] = "";
            int playnow = 0;
            unsigned char cbmove[512];
            return getmove(b, 2, 0.1, str, &playnow, 1, 0, cbmove);
        };
        const int lost = claim_from("B:W10:B2"), blocked = claim_from("B:W6,7,9,10,11:B2");
        if (lost != 3 || blocked != 2) {
            std::cerr << "a lost position claims " << lost << " (3 = unknown), no legal move " << blocked
                      << " (2 = loss)\n";
            return 1;
        }
    }

    // Claims hold under the game rules with the history CheckerBoard sends (review A round 6). getmove from FEN after
    // "set gamehist HIST" (none if empty); the return value, or -1 if the move played is not WANT (when given).
    auto claim = [&](const char* fen, const std::string& hist, const char* want = nullptr) {
        Position p;
        p.set_fen(fen);
        if (!hist.empty() && command(("set gamehist " + hist).c_str(), reply) != 1) return -1;
        Board8x8 b, expected;
        to_board(p, b);
        char str[1024] = "";
        int playnow = 0;
        unsigned char cbmove[512];
        const int c = getmove(b, p.side_to_move() == WHITE ? 1 : 2, 0.1, str, &playnow, 1, 0, cbmove);
        Move m;
        if (want) {
            if (!parse_move(p, want, m)) return -1;
            p.make_move(m);
            to_board(p, expected);
            if (std::memcmp(b, expected, sizeof(Board8x8)) != 0) return -1;
        }
        return c;
    };
    // Evaluation, book and avoid list as in the review (HCE, both off), set back afterwards.
    {
        if (command("get book", reply) != 1) return 1;
        const std::string book = reply;
        command("set evalfile hce", reply);
        command("set book 0", reply);
        command("set avoid 0", reply);
        // An immediate win is claimed (1).
        const int win = claim("B:W10:B6", "", "6x15");
        // The root is the third occurrence of the position: a draw already, no win claim, although the search
        // scores a win from the board.
        const std::string cycle = "W:WK2,K8,K12,K18,K20,K28,K29:BK13,K30 28-24 13-17 24-28 17-13 28-24 13-17 24-28";
        const int drawn_root = claim("W:WK2,K8,K12,K18,K20,K28,K29:BK13,K30", cycle + " 17-13");
        // One ply earlier, 17-13 makes that third occurrence: a draw claim (0).
        const int draw = claim("B:WK2,K8,K12,K18,K20,K28,K29:BK17,K30", cycle, "17-13");
        command("set evalfile builtin", reply);
        command(("set book " + book).c_str(), reply);
        command("set avoid 1", reply);
        std::cout << "claims: immediate win " << win << " (1), third-occurrence root " << drawn_root
                  << " (not 1), 17-13 completing it " << draw << " (0)\n";
        if (win != 1 || drawn_root == 1 || draw != 0) return 1;
    }

    Position pos;
    int plies = 0;
    for (; plies < 200; ++plies) {
        MoveList list;
        generate_moves(pos, list);
        if (list.empty()) break;
        Board8x8 b;
        to_board(pos, b);
        char str[1024] = "";
        int playnow = 0;
        unsigned char cbmove[512];
        getmove(b, pos.side_to_move() == WHITE ? 1 : 2, 0.01, str, &playnow, plies == 0 ? 1 : 0, 0, cbmove);

        bool found = false;
        for (const Move& m : list) {
            Position next = pos;
            next.make_move(m);
            Board8x8 nb;
            to_board(next, nb);
            if (std::memcmp(nb, b, sizeof(Board8x8)) == 0) {
                pos = next;
                found = true;
                break;
            }
        }
        if (!found) {
            std::cerr << "ply " << plies << ": engine returned an illegal board from " << pos.fen() << '\n';
            return 1;
        }
    }
    std::cout << "played " << plies << " legal plies, final " << pos.fen() << '\n';

    // Database path: the first directory loads; the same directory under another spelling is accepted again; a
    // different one is refused (the tables of a running engine stay), and `get dbpath` still names the first.
    {
        const std::string dir = test_egdb::build("cb-api-test", 3);
        const std::string other = (std::filesystem::temp_directory_path() / "regicide-no-such-egdb").string();
        const bool ok = !dir.empty() && command(("set dbpath " + dir).c_str(), reply) == 1 && std::string(reply) == "3" &&
                        command(("set dbpath " + dir + "/.").c_str(), reply) == 1 &&
                        command(("set dbpath " + other).c_str(), reply) == 0 && command("get dbpath", reply) == 1 &&
                        std::string(reply) == dir && command("get dbpieces", reply) == 1 && std::string(reply) == "3";
        if (!ok) {
            test_egdb::remove(dir);
            std::cerr << "set dbpath failed: " << reply << '\n';
            return 1;
        }
        // B:WK3:BK1,K2 is a table win with MTC 6 (score 19994). After the 77 king moves of review A round 6 the
        // conversion comes too late for the no-progress limit: no win claim. Without that history it is claimed.
        const std::string hist77 =
            "W:WK23:BK1,K2 23-19 2-6 19-15 6-2 15-18 1-5 18-15 2-6 15-11 6-1 11-16 5-9 16-19 1-5 19-23 5-1 23-18 1-5 "
            "18-22 9-6 22-26 6-9 26-30 9-6 30-25 6-2 25-22 5-9 22-25 9-14 25-30 14-17 30-26 17-21 26-23 2-6 23-26 "
            "21-17 26-31 6-1 31-27 1-5 27-23 17-21 23-26 21-25 26-31 5-1 31-27 25-21 27-23 21-17 23-19 17-14 19-15 "
            "14-17 15-10 1-5 10-7 17-13 7-10 13-9 10-15 5-1 15-10 9-5 10-7 5-9 7-3 1-6 3-7 9-5 7-3 5-1 3-7 6-2 7-3";
        const int late = claim("B:WK3:BK1,K2", hist77), fresh = claim("B:WK3:BK1,K2", "");
        // B:WK10,K30:BK1,K6 has four pieces, outside the tables; its only move 6x15 reaches a 3-piece table win
        // (score SCORE_DB_WIN - 1, no distance). Claimed after 4 king moves; not after 38, which with that ply
        // reach half the no-progress window (38 + 1 >= 79 / 2).
        const int searched = claim("B:WK10,K30:BK1,K6", "B:WK3,K30:BK1,K6 1-5 3-7 5-1 7-10", "6x15");
        const int searched_late = claim(
            "B:WK10,K30:BK1,K6",
            "B:WK5,K21:BK6,K7 6-1 5-9 7-10 9-5 1-6 5-1 6-2 21-25 2-6 1-5 6-2 5-9 10-6 9-5 6-1 5-9 1-5 9-14 5-1 14-10 "
            "2-6 10-7 1-5 7-3 5-1 3-8 1-5 8-4 5-1 25-30 1-5 4-8 5-1 8-3 1-5 3-7 5-1 7-10",
            "6x15");
        // Review A round 7: B:WK3:BK1,K2 for the second time; after 1-5 White's 3-8 makes B:WK8:BK2,K5 for the
        // third time, a draw: no win claim (and 1-5 is still the move played).
        const int reply_draw = claim("B:WK3:BK1,K2", "B:WK8:BK2,K5 5-1 8-3 1-5 3-8 5-1 8-3", "1-5");
        test_egdb::remove(dir);
        std::cout << "table win claims: after 77 king moves " << late << " (not 1), without history " << fresh
                  << " (1); searched table win after 4 king moves " << searched << " (1), after 38 " << searched_late
                  << " (not 1); the reply can repeat a third time " << reply_draw << " (not 1, not -1)\n";
        if (late == 1 || fresh != 1 || searched != 1 || searched_late == 1 || reply_draw == 1 || reply_draw == -1)
            return 1;
    }
    return 0;
}
