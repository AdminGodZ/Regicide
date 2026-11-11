#!/usr/bin/env bash
# (b) "The result" (lead 2026-11-11 11:57): one concurrency-1 run per opponent on master 8de39d0 as committed (DLL
# bl-8de39d0), --pairs-together, processes kept, the production deck in its order (312 games), --cb-claims (CheckerBoard's
# claim handling with early adjudication) and --cb-move-limit 200; no timing lock (one game at a time), its own host copy
# (C:\regicide-match\bin\8de39d0-c1-OPP) so that affinity_keeper.ps1 pins its two engines to one core. Starts after the
# claims census (CL done).   mo2_run.sh OPP  ->  bl-mo2-OPP
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; OPP=$1
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "^CL done\|^CL STOPPED" $L/status; do sleep 30; done
grep -q "^CL done" $L/status || { st "MO2 $OPP not started (CL stopped)"; exit 1; }
export MATCH=$L/bin/match-8de3 CBHOST=/mnt/c/regicide-match/bin/8de39d0-c1-$OPP/cbhost.exe
cmp -s $CBHOST $L/bin/8de3/cbhost.exe || { st "MO2 $OPP refused: host copy differs"; exit 1; }
NOLOCK=1 EXTRA_ARGS="--allow-rejected-options --pairs-together --cb-claims --cb-move-limit 200" \
  bash $J/bl_run_w.sh regicide:bl-8de39d0 $OPP mo2-$OPP $J/acf156_all.txt 0.5 1 < /dev/null && st "MO2 $OPP done" || st "MO2 $OPP STOPPED"
