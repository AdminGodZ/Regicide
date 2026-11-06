// Loads the CheckerBoard engine library and plays a self-play game through getmove(), checking that every
// returned board is reachable from the previous one by exactly one legal move.
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstring>
#include <iostream>

#include "movegen.hpp"
#include "position.hpp"

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
    return 0;
}
