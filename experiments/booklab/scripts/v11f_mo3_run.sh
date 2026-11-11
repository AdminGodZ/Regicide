#!/usr/bin/env bash
# r11f validation b) (lead 2026-11-11 14:32): one concurrency-1 run per opponent exactly as mo2 (pairs together, kept
# processes, the production deck in its order, --cb-claims --cb-move-limit 200, no timing lock, one game at a time),
# DLL bl-fdd154e-r11f, runner bin/match-fdd1, its own host copy C:\regicide-match\bin\fdd154e-c1-OPP (pinned to one core
# by affinity_keeper.ps1).   v11f_mo3_run.sh OPP  ->  bl-mo3-OPP
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; OPP=$1
st() { echo "$* $(date -Is)" >> $L/status; }
export MATCH=$L/bin/match-fdd1 CBHOST=/mnt/c/regicide-match/bin/fdd154e-c1-$OPP/cbhost.exe
cmp -s $CBHOST $L/bin/fdd1/cbhost.exe || { st "MO3 $OPP STOPPED: host copy differs"; exit 1; }
NOLOCK=1 EXTRA_ARGS="--allow-rejected-options --pairs-together --cb-claims --cb-move-limit 200" \
  bash $J/bl_run_w.sh regicide:bl-fdd154e-r11f $OPP mo3-$OPP $J/acf156_all.txt 0.5 1 < /dev/null && st "MO3 $OPP done" || st "MO3 $OPP STOPPED"
