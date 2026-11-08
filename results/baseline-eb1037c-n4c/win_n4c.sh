#!/usr/bin/env bash
# Baseline engine: eb1037c, FT 1024 (CMake option), n4c embedded as src/weights/net.bin. Windows DLL + Linux checks.
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
N4C=~/regicide-nnue4-work/n4c.nnue
SHA=eb1037c OUT=eb1037c-n4c NETFILE=$N4C CMAKE_EXTRA="-DREGICIDE_NNUE_FT=1024" JOBS=12 \
  bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh
BL=~/regicide-match/builds/eb1037c-n4c/build-linux
echo "-- linux ctest (nnue_test expects the built-in net to be rejected in custom-width builds, so it fails here by design)"
ctest --test-dir $BL -j 8 2>&1 | tail -n 6
echo "-- reference: 385611e build-ft1024 + set evalfile n4c"
printf "set evalfile $N4C\nbench\nquit\n" | ~/regicide-385/build-ft1024/regicide | grep '^bench'
echo "-- eb1037c FT1024 linux build with set evalfile n4c (same as embedded?)"
printf "set evalfile $N4C\nbench\nquit\n" | $BL/regicide | grep '^bench'
st "WIN_N4C_DONE"
