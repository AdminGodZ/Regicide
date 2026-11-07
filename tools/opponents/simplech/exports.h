/* Give the engine entry points C linkage: a later definition without a linkage specification
 * keeps the linkage of this first declaration. (On Windows a .def file does this job.) */
#pragma once
#include "cb_interface.h"
extern "C" {
int getmove(Board8x8 b, int color, double maxtime, char str[255], int* playnow, int info, int moreinfo, CBmove* move);
int enginecommand(const char* str, char reply[256]);
}
