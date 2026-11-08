#!/usr/bin/env bash
# Run inside an attached wsl.exe (Start-Process hidden). 8-piece EGDB baseline: DLL 30f09c3-egdb8 (the 30f09c3 DLL of
# the 7-piece baseline + a 2-8 piece egdb copy) vs KingsRow, Cake, GuiNN with the settings of baseline-eb1037c-n4c.
# Waits for any A/B match (matchlock), then holds baselock (pausekeeper4 pauses bookgen; datagen keeps running
# alongside as in the 7-piece baseline).
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
while [ -e $J/matchlock ]; do sleep 20; done
touch $J/baselock; sleep 8
st "EGDB8BASE start: baselock taken; $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
NAME=30f09c3-egdb8 RUNS="kingsrow cake guinn" bash /mnt/c/regicide-match/jobs/r4/run_baseline_r4.sh
rm -f $J/baselock
st "EGDB8BASE_DONE"
