#!/usr/bin/env bash
# T1 (experiments/time-policy/README.md + the lead's addition, 2026-11-11): tm_soft 60 against 100 (set param, read back),
# book r10k5 (DLL bl-141a025-r10k5, engine master 141a025), net n6c, one thread, hash 128, book mode 3 for both engines,
# runner and host from master 162ecac with the per-move log (bin/match-m162-movelog = tools/match of 162ecac +
# tools/booklab/patches/match_movelog.diff; bin/m162ecac/cbhost.exe): Cake on holdout H3 (both colours, 312 games per
# arm) and KingsRow mode 3 on the production deck (312 games per arm). Arms alternate by part.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
st "T1 start (r10k5 on 141a025; runner and host 162ecac)"
export MATCH=$L/bin/match-m162-movelog EXTRA_ARGS=--allow-rejected-options CBHOST=$L/bin/m162/cbhost.exe
run() {  # run ARM OPP PART DECK
  EXTRA_A="set param tm_soft=$1" bash $J/bl_run_w.sh regicide:bl-141a025-r10k5 $2 t1a-$1-$2-$3 $4 0.5 8 < /dev/null \
    || { st "T1 STOPPED at t1a-$1-$2-$3"; exit 1; }
}
run 60 cake H3_0 $L/wp10/parts/H3_0.txt;   run 100 cake H3_0 $L/wp10/parts/H3_0.txt
run 100 kingsrow h1 $J/acf156_h1.txt;      run 60 kingsrow h1 $J/acf156_h1.txt
run 100 cake H3_1 $L/wp10/parts/H3_1.txt;  run 60 cake H3_1 $L/wp10/parts/H3_1.txt
run 60 kingsrow h2 $J/acf156_h2.txt;       run 100 kingsrow h2 $J/acf156_h2.txt
st "T1 done (r10k5)"
