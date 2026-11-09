#!/usr/bin/env bash
# Targeted test of a candidate DLL: each listed ballot N times (both colours each, so N games per colour) against one
# opponent, 0.5 s/move, baseline settings. Takes the timing lock (via bl_run.sh).
#   bl_target.sh DLLDIR TAG OPP N "BALLOT MOVES" ["BALLOT MOVES" ...]
# e.g. bl_target.sh bl-r1 t1-cake cake 10 "11-16 23-19 16x23" "9-13 22-18 10-15"
set -u
DLL=$1; TAG=$2; OPP=$3; N=$4; shift 4
L=~/regicide-jobs/booklab; mkdir -p $L/decks
DECK=$L/decks/$TAG.txt; : > $DECK
for b in "$@"; do for i in $(seq $N); do echo "$b" >> $DECK; done; done
bash /mnt/c/regicide-match/jobs/booklab/bl_run.sh "regicide:$DLL" "$OPP" "$TAG" "$DECK" 0.5 8 < /dev/null
