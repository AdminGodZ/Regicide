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
        const std::string dir = test_egdb::build("cb-api-test", 2);
        const std::string other = (std::filesystem::temp_directory_path() / "regicide-no-such-egdb").string();
        const bool ok = !dir.empty() && command(("set dbpath " + dir).c_str(), reply) == 1 && std::string(reply) == "2" &&
                        command(("set dbpath " + dir + "/.").c_str(), reply) == 1 &&
                        command(("set dbpath " + other).c_str(), reply) == 0 && command("get dbpath", reply) == 1 &&
                        std::string(reply) == dir && command("get dbpieces", reply) == 1 && std::string(reply) == "2";
        test_egdb::remove(dir);
        if (!ok) {
            std::cerr << "set dbpath failed: " << reply << '\n';
            return 1;
        }
    }
    return 0;
}
