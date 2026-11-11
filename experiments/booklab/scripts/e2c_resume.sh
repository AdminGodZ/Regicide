#!/usr/bin/env bash
# Experiment 2 deck arm, resumed after 2b (the chain e2_chain.sh died at e2c-kept-2-h2 on 2026-11-11 08:58: its
# bl_run_w.sh was replaced by mv while running and bash lost the script; that half-run's match had played its games
# but has no exit_status, so it is replayed). Same settings as e2_chain.sh: DLL bl-ddd25fc-r11d, runner and host from
# ddd25fc, concurrency 8, finished half-runs are skipped by bl_run_w.sh.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "MO 2b done\|MO STOPPED" $L/status; do sleep 60; done
export MATCH=$L/bin/match-ddd2 CBHOST=$L/bin/ddd2/cbhost.exe
st "E2 resumed (deck arm)"
for i in 1 2 3 4 5 6; do
  for arm in kept fresh; do
    X="--allow-rejected-options"; [ $arm = fresh ] && X="$X --fresh-engines"
    for h in h1 h2; do
      EXTRA_ARGS="$X" bash $J/bl_run_w.sh regicide:bl-ddd25fc-r11d cake e2c-$arm-$i-$h $J/acf156_$h.txt 0.5 8 < /dev/null \
        || { st "E2 STOPPED at e2c-$arm-$i-$h (resumed chain)"; exit 1; }
    done
  done
done
st "E2 done"
