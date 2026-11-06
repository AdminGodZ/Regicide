/* Minimal POSIX stand-ins for the Win32 bits that CheckerBoard engines use.
 * The build scripts generate stub <windows.h>, <conio.h>, ... headers that include this file. */
#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define WINAPI
#define CALLBACK
#define NOMINMAX

typedef int BOOL;
typedef int INT;
typedef unsigned int UINT;
typedef uint32_t DWORD;
typedef unsigned char BYTE;
typedef void* HANDLE;
typedef void* LPVOID;
typedef const char* LPCSTR;

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif
#ifndef MAX_PATH
#define MAX_PATH 260
#endif
#define DLL_PROCESS_DETACH 0
#define DLL_PROCESS_ATTACH 1
#define DLL_THREAD_ATTACH 2
#define DLL_THREAD_DETACH 3

static inline void Sleep(DWORD ms) { usleep((useconds_t)ms * 1000); }

/* CheckerBoard engines time their searches with clock(). On Windows that is wall time; on
 * POSIX it is CPU time. Engines are tuned for (and matches are refereed in) wall time. */
#define CLK_TCK CLOCKS_PER_SEC
static inline clock_t cbport_wallclock(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (clock_t)((uint64_t)ts.tv_sec * CLOCKS_PER_SEC + (uint64_t)ts.tv_nsec / (1000000000 / CLOCKS_PER_SEC));
}
#ifndef CBPORT_KEEP_CLOCK
#define clock() cbport_wallclock()
#endif

/* <windows.h> min/max (without NOMINMAX); engines that use std::min/max define CBPORT_NO_MINMAX. */
#if defined(__cplusplus) && !defined(CBPORT_NO_MINMAX)
#include <algorithm>
using std::max;
using std::min;
#endif
