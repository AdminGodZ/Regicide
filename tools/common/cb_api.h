#pragma once

// The CheckerBoard engine API (https://github.com/eygilbert/CheckerBoard, source/cb_interface.h),
// restated for POSIX hosts. On macOS/Linux WINAPI is the default C calling convention, so engines
// built as .dylib/.so export plain C symbols:
//
//   int getmove(int board[8][8], int color, double maxtime, char str[1024], int *playnow,
//               int info, int moreinfo, struct CBmove *move);
//   int enginecommand(const char *command, char reply[1024]);
//
// board[x][y] uses CheckerBoard's English numbering (square 1 at x=6,y=0; see cbt::square_xy).

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CB_FREE 0
#define CB_WHITE 1
#define CB_BLACK 2
#define CB_MAN 4
#define CB_KING 8

/* getmove() return values */
#define CB_DRAW 0
#define CB_WIN 1
#define CB_LOSS 2
#define CB_UNKNOWN 3

/* getmove() 'info' bits */
#define CB_RESET_MOVES 1
#define CB_EXACT_TIME 2
/* Bits 2..3 of 'info': incremental time units (0 = off, 1 = 1 ms, 2 = 10 ms, 3 = 100 ms).
 * 'moreinfo' then holds remaining time in its upper 16 bits and the increment in the lower 16. */
#define CB_INCR_TIME_SHIFT 2

#define GT_ENGLISH 21

struct CBcoor {
    int x;
    int y;
};

struct CBmove {
    int jumps;               /* number of pieces jumped */
    int newpiece;            /* piece type that lands on the 'to' square */
    int oldpiece;            /* piece type that leaves the 'from' square */
    struct CBcoor from, to;  /* Board8x8 coordinates */
    struct CBcoor path[12];  /* intermediate landing squares, starting at path[1] */
    struct CBcoor del[12];   /* squares of captured pieces */
    int delpiece[12];        /* captured piece types */
};

typedef int (*CB_GETMOVE)(int board[8][8], int color, double maxtime, char str[1024], int* playnow, int info,
                          int moreinfo, struct CBmove* move);
typedef int (*CB_ENGINECOMMAND)(const char* command, char reply[1024]);

#ifdef __cplusplus
}

namespace cbt {

// CheckerBoard's format_time_args(): pack increment and remaining time (seconds) into
// info/moreinfo using the finest unit that fits in 16 bits.
inline void cb_format_time_args(double increment, double remaining, int* info, int* moreinfo) {
    const double mult[] = {0.001, 0.01, 0.1};
    const double limit = 65535 * 0.1;
    if (remaining < 0.005) remaining = 0.005;
    double largest = increment > remaining ? increment : remaining;
    if (largest > limit) {
        largest = limit;
        if (increment > limit) increment = limit;
        if (remaining > limit) remaining = limit;
    }
    for (int i = 0; i < 3; ++i) {
        if (largest <= 65535 * mult[i]) {
            uint16_t inc16 = (uint16_t)(increment / mult[i]);
            uint16_t rem16 = (uint16_t)(remaining / mult[i]);
            *moreinfo = (int)(((uint32_t)rem16 << 16) | inc16);
            *info |= (i + 1) << CB_INCR_TIME_SHIFT;
            return;
        }
    }
}

// CheckerBoard's maxtime_for_incremental_tc(): the 'maxtime' sent to engines that ignore the
// packed incremental times.
inline double cb_maxtime_for_incremental_tc(double remaining, double initial_time, double increment) {
    double divisor;
    if (remaining < 0.4 * increment) return 0.4 * increment;
    if (remaining <= increment) return remaining;
    if (increment == 0) divisor = 11;
    else if (initial_time / increment >= 50) divisor = 10;
    else if (initial_time / increment >= 20) divisor = 9;
    else if (initial_time / increment >= 10) divisor = 7;
    else if (initial_time / increment >= 5) divisor = 5;
    else divisor = 4;
    double t = increment + remaining / divisor;
    return t < remaining ? t : remaining;
}

// CheckerBoard's maxtime_for_non_incremental_tc(): fixed time per move in an engine match,
// where unused time is banked (remaining already includes this move's allotment).
inline double cb_maxtime_for_non_incremental_tc(double remaining, double increment) {
    if (remaining < 0.4 * increment) return 0.4 * increment;
    if (remaining > increment) return increment + (remaining - increment) / 2.5;
    return remaining;
}

}  // namespace cbt
#endif
