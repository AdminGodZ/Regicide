#!/usr/bin/env bash
# One of the three concurrency-1 match-order runs (lead 2026-11-11, item 3): master 1c6e997 as committed (DLL
# bl-1c6e997, runner bin/match-1c6e), --pairs-together, processes kept, the production deck in its order (312 games),
# no timing lock (NOLOCK; one game at a time), its own copy of the host so that affinity_keeper.ps1 can pin its two
# engines to one core.   mo3_run.sh OPP   ->  bl-mo1-OPP
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; OPP=$1
st() { echo "$* $(date -Is)" >> $L/status; }
export MATCH=$L/bin/match-1c6e CBHOST=/mnt/c/regicide-match/bin/1c6e997-c1-$OPP/cbhost.exe
cmp -s $CBHOST $L/bin/1c6e/cbhost.exe || { st "MO1 $OPP refused: host copy differs"; exit 1; }
NOLOCK=1 EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh regicide:bl-1c6e997 $OPP mo1-$OPP $J/acf156_all.txt 0.5 1 < /dev/null \
  && st "MO1 $OPP done" || st "MO1 $OPP STOPPED"
