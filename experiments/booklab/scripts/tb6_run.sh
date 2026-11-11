#!/usr/bin/env bash
# Test bed for the #6 Black repair (lead 2026-11-11 12:35): the deck prefix up to and including ACF #6 (the first five
# ballots of acf156_all.txt = games 1 to 10, the loss was game 9 = round 5.1), same DLL (bl-1c6e997), runner
# (match-1c6e), settings and order as mo1-kingsrow (--pairs-together, processes kept, concurrency 1, KingsRow book mode
# 3), played REPS times one after the other (new processes at each start, as at mo1-kingsrow's start); own host copy
# (1c6e997-c1-tb, pinned to core 20 by affinity_keeper.ps1); no timing lock.   tb6_run.sh [REPS] [DLL] [TAGPREFIX]
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; REPS=${1:-6}; DLL=${2:-bl-1c6e997}; P=${3:-tb6}
st() { echo "$* $(date -Is)" >> $L/status; }
head -5 $J/acf156_all.txt > $L/tb6_deck.txt
export MATCH=$L/bin/match-1c6e CBHOST=/mnt/c/regicide-match/bin/1c6e997-c1-tb/cbhost.exe
cmp -s $CBHOST $L/bin/1c6e/cbhost.exe || { st "TB6 refused: host copy differs"; exit 1; }
for i in $(seq $REPS); do
  NOLOCK=1 EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh regicide:$DLL kingsrow $P-$i $L/tb6_deck.txt 0.5 1 < /dev/null \
    || { st "TB6 STOPPED at $P-$i"; exit 1; }
done
st "TB6 done ($P)"
