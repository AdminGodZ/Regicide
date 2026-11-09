#!/usr/bin/env bash
# Run inside an attached wsl.exe. More deep-book tests on the same DLL (ff0a3d2-deep-egdb8, book off = "set book 0"),
# baseline settings (acf156, 312 games, 0.5 s/move, -c 8; gen5c datagen alongside):
#  1. Cake: off, on, off, on (pairs 2 and 3; pair 1 = deep-off-cake / deep-on-cake)
#  2. KingsRow off, GuiNN off (same-engine references for the book-on runs)
J=~/regicide-jobs/r4; R=/mnt/c/regicide-match/jobs/ba/run_ba2.sh; D=ff0a3d2-deep-egdb8
st() { echo "DEEP $* $(date -Is)" >> $J/status; }
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
st "more tests start"
bash $R $D deep-off2 cake 0.5 "set book 0"
bash $R $D deep-on2 cake 0.5
bash $R $D deep-off3 cake 0.5 "set book 0"
bash $R $D deep-on3 cake 0.5
st "MORE_CAKE_DONE"
bash $R $D deep-off kingsrow 0.5 "set book 0"
bash $R $D deep-off guinn 0.5 "set book 0"
st "DEEP_MORE_DONE"
