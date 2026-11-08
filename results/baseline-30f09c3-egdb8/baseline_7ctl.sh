#!/usr/bin/env bash
# Run inside an attached wsl.exe. Control run for the 8-piece baseline: the 7-piece DLL (engines/regicide/30f09c3,
# 2-7 piece egdb, the same regicide_cb.dll) vs KingsRow under the 8-piece baseline's conditions (gen5 50k-node datagen
# at 12 threads alongside, bookgen paused via baselock, gen4b held). Separates the database's cost from the load's.
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
until grep -q '^EGDB8 nps_check done' $J/status; do sleep 10; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 10; done
touch $J/baselock; sleep 8
st "EGDB8CTL start: baselock taken; $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
NAME=30f09c3 RUNS="kingsrow" bash /mnt/c/regicide-match/jobs/r4/run_baseline_r4.sh
rm -f $J/baselock
st "EGDB8CTL_DONE"
