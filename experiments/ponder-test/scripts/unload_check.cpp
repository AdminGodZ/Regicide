// unload_check DLL: Windows FreeLibrary while the ponder thread runs.
// 1. load, ponder 1, threads 4, one getmove (0.5 s) -> ponder thread starts
// 2. FreeLibrary: must return at once; the module stays mapped while the ponder thread holds its reference
// 3. poll GetModuleHandle until the module is really unmapped (the ponder thread exits on its own cap)
// 4. reload: fresh state (get ponder == 0), a move works; repeat with a reload while still pinned
#include <windows.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

using GetMove = int (*)(int[8][8], int, double, char*, int*, int, int, void*);
using Cmd = int (*)(const char*, char*);
using Clock = std::chrono::steady_clock;

static double ms_since(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

static void start_board(int b[8][8]) {
    std::memset(b, 0, sizeof(int) * 64);
    for (int pdn = 1; pdn <= 32; ++pdn) {
        const int n = pdn - 1, y = n / 4, x = 2 * (3 - n % 4) + (y & 1);
        if (pdn <= 12) b[x][y] = 2 | 4;
        if (pdn >= 21) b[x][y] = 1 | 4;
    }
}

struct Lib {
    HMODULE h = nullptr;
    GetMove getmove = nullptr;
    Cmd cmd_ = nullptr;
    bool open(const char* path) {
        h = LoadLibraryA(path);
        if (!h) return false;
        getmove = reinterpret_cast<GetMove>(reinterpret_cast<void*>(GetProcAddress(h, "getmove")));
        cmd_ = reinterpret_cast<Cmd>(reinterpret_cast<void*>(GetProcAddress(h, "enginecommand")));
        return getmove && cmd_;
    }
    std::string cmd(const char* c) {
        char r[1024] = "";
        cmd_(c, r);
        return r;
    }
    std::string move(double t, int info) {
        int b[8][8];
        start_board(b);
        char str[1024] = "";
        int playnow = 0;
        unsigned char mv[512];
        getmove(b, 2, t, str, &playnow, info, 0, mv);
        return str;
    }
};

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    const char* path = argv[1];
    const char* base = std::strrchr(path, '\\') ? std::strrchr(path, '\\') + 1 : path;
    bool ok = true;
    for (int round = 0; round < 2; ++round) {
        Lib lib;
        if (!lib.open(path)) { std::printf("cannot load\n"); return 1; }
        std::printf("round %d: get ponder after load = %s\n", round, lib.cmd("get ponder").c_str());
        if (lib.cmd("get ponder") != "0") ok = false;
        lib.cmd("set ponder 1");
        lib.cmd("set threads 4");
        lib.cmd("set hashsize 512");
        lib.cmd("set book 0");
        const std::string info = lib.move(0.5, 1);
        std::printf("  move: %s\n", info.c_str());
        std::string pp;
        for (int i = 0; i < 2000 && (pp = lib.cmd("get ponderpos")).empty(); ++i) Sleep(1);
        std::printf("  pondering: %s\n", pp.c_str());
        if (pp.empty()) ok = false;
        Sleep(300);
        auto t0 = Clock::now();
        FreeLibrary(lib.h);
        std::printf("  FreeLibrary returned after %.2f ms\n", ms_since(t0));
        if (ms_since(t0) > 100) ok = false;
        const bool still = GetModuleHandleA(base) != nullptr;
        std::printf("  module still mapped right after FreeLibrary: %s\n", still ? "yes" : "no");
        if (round == 1) {
            // reload while the old thread may still be pondering: same instance, must keep working
            Lib again;
            if (!again.open(path)) { std::printf("cannot reload\n"); return 1; }
            std::printf("  reload while pinned: get ponder = %s\n", again.cmd("get ponder").c_str());
            again.cmd("set ponder 0");
            auto t1 = Clock::now();
            const std::string i2 = again.move(0.3, 1);
            std::printf("  move after reload (%.0f ms): %s\n", ms_since(t1), i2.c_str());
            if (ms_since(t1) > 1200 || i2.empty()) ok = false;
            FreeLibrary(again.h);
        }
        t0 = Clock::now();
        while (GetModuleHandleA(base) && ms_since(t0) < 30000) Sleep(10);
        const bool gone = GetModuleHandleA(base) == nullptr;
        std::printf("  module unmapped: %s, %.0f ms after FreeLibrary\n", gone ? "yes" : "NO", ms_since(t0));
        if (!gone) ok = false;
    }
    std::printf(ok ? "unload_check passed\n" : "unload_check FAILED\n");
    return ok ? 0 : 1;
}
