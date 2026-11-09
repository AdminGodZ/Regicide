#!/usr/bin/env bash
# Run inside an attached wsl.exe. Install checks for book_deep2b (DLL 6bb8c6c-deep2b-egdb8), settings of the deep-book
# tests (acf156, 312 games, 0.5 s/move, -c 8, 2-8 EGDB): one full run vs Cake, then one vs KingsRow (back to back).
J=~/regicide-jobs/r4; R=/mnt/c/regicide-match/jobs/deep2/run_ba3.sh
st() { echo "D2 $* $(date -Is)" >> $J/status; }
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 10; done
unset OPENINGS
st "install checks start (book_deep2b, acf156)"
bash $R 6bb8c6c-deep2b-egdb8 deep2b-full cake 0.5
bash $R 6bb8c6c-deep2b-egdb8 deep2b-full kingsrow 0.5
st "D2B_FULL_DONE"
