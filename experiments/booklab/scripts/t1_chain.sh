#!/usr/bin/env bash
# Lead order 2026-11-11 02:00: (1) targeted test of r10k5 (DLL from master 141a025) on all repaired lines (doubles as the
# five-opening check that the new base changed nothing): ACF #60, #71, #34, #141, #9, #62, #93 (Regicide Black), #53
# (White): 10 games against KingsRow mode 3, 10 against Cake, 4 against GuiNN; no loss allowed.
# (2) T1 (experiments/time-policy/README.md + the lead's addition): tm_soft 60 against 100 (set param, read back), book
# r10k5, net n6c, one thread, hash 128, book mode 3 for both engines, runner and host from master 141a025 with the
# per-move log (bin/match-m141-movelog, bin/m141a025/cbhost.exe): Cake on holdout H3 (both colours, 312 games per arm)
# and KingsRow mode 3 on the production deck (312 games per arm); arms alternate by part.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results
st() { echo "$* $(date -Is)" >> $L/status; }
losses() { for d in "$@"; do [ -f $d/summary.txt ] || { echo 999; continue; }
  sed -n 's/^A: +[0-9]* =[0-9]* -\([0-9]*\).*/\1/p' $d/summary.txt; done | awk '{s += $1} END {print s + 0}'; }
DLLPREFIX=bl-141a025 NCAKE=10 bash $J/wp13_test.sh r10k5 5 60,71,34,141,9,62,93,53 < /dev/null || { st "T1CHAIN STOPPED in test 5"; exit 1; }
n=$(losses $R/bl-w13-t5-r10k5-kingsrow $R/bl-w13-t5-r10k5-cake $R/bl-w13-t5-r10k5-guinn)
[ "$n" = 0 ] || { st "T1CHAIN STOPPED: test 5 has $n losses"; exit 1; }
st "T1 start (r10k5, master 141a025)"
export MATCH=$L/bin/match-m141-movelog EXTRA_ARGS=--allow-rejected-options CBHOST=$L/bin/m141/cbhost.exe
run() {  # run ARM OPP PART DECK
  EXTRA_A="set param tm_soft=$1" bash $J/bl_run_w.sh regicide:bl-141a025-r10k5 $2 t1a-$1-$2-$3 $4 0.5 8 < /dev/null \
    || { st "T1 STOPPED at t1a-$1-$2-$3"; exit 1; }
}
run 60 cake H3_0 $L/wp10/parts/H3_0.txt;   run 100 cake H3_0 $L/wp10/parts/H3_0.txt
run 100 kingsrow h1 $J/acf156_h1.txt;      run 60 kingsrow h1 $J/acf156_h1.txt
run 100 cake H3_1 $L/wp10/parts/H3_1.txt;  run 60 cake H3_1 $L/wp10/parts/H3_1.txt
run 60 kingsrow h2 $J/acf156_h2.txt;       run 100 kingsrow h2 $J/acf156_h2.txt
st "T1 done (r10k5)"
