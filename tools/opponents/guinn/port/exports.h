// Give GuiNN's CheckerBoard entry points C linkage: a later definition without a linkage
// specification keeps the linkage of this first declaration. (On Windows guiCheckers.def does it.)
#pragma once
#include "cb_interface.h"
extern "C" {
int getmove(int board[8][8], int color, double maxtime, char str[1024], int* playnow, int info, int moreinfo,
            struct CBmove* move);
int enginecommand(char str[256], char reply[1024]);
}
