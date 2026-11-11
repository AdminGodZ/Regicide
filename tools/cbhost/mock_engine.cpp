// A CheckerBoard-API engine for the host's own tests (cbhost_test). It plays the first legal move.
//   set histdelay N   `set gamehist ...` takes N ms (an engine that is slow to take the game history)
//   set think N       getmove works for N ms, or until *playnow is set
// The info string reports whether the stop flag was already set when getmove was entered.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>

#include "cb_api.h"
#include "checkers.hpp"

namespace {
int hist_delay_ms = 0;
int think_ms = 0;
}  // namespace

extern "C" int enginecommand(const char* command, char* reply) {
    const std::string s(command);
    reply[0] = 0;
    if (s == "name") {
        std::strcpy(reply, "cbhost mock engine");
        return 1;
    }
    if (s.rfind("set histdelay ", 0) == 0) {
        hist_delay_ms = std::stoi(s.substr(14));
        return 1;
    }
    if (s.rfind("set think ", 0) == 0) {
        think_ms = std::stoi(s.substr(10));
        return 1;
    }
    if (s.rfind("set gamehist ", 0) == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(hist_delay_ms));
        return 1;
    }
    return 0;
}

extern "C" int getmove(int board[8][8], int color, double, char* str, int* playnow, int, int, CBmove*) {
    const volatile int* stop = playnow;
    const int stopped_on_entry = *stop ? 1 : 0;
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(think_ms);
    while (!*stop && std::chrono::steady_clock::now() < end) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    const cbt::Board before = cbt::Board::from_board8(board, color == CB_BLACK ? cbt::Color::Black : cbt::Color::White);
    const auto legal = before.legal_moves();
    if (legal.empty()) return CB_LOSS;
    before.after(legal[0]).to_board8(board);
    std::snprintf(str, 1024, "stopped_on_entry %d", stopped_on_entry);
    return CB_UNKNOWN;
}
