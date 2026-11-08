#!/usr/bin/env bash
# Why did Regicide's searched-move nps drop from ~3.7M (7-piece baseline) to ~2.2M (8-piece baseline)?
# Same 120 game positions, Windows regicide.exe (30f09c3), 8 processes at once, 2-7 vs 2-8 database, under:
#   (a) quiet: matchlock (pausekeeper4 stops gen4b, gen5, bookgen)
#   (b) gen5 only (50k-node datagen, 12 threads), as during the 8-piece baseline: baselock + gen4bhold
# (A gen4b-only case would need a lock that stops gen5 but not gen4b; pausekeeper4 has none, so it is left out.)
# Waits for any A/B match to finish first. Output: ~/regicide-jobs/egdb8/nps_check.txt
J=~/regicide-jobs/r4; JE=~/regicide-jobs/egdb8; P=/mnt/c/Users/micro/AppData/Local/Temp/claude/80b136091be57ac0/9c386aa2-abab-4bf3-914c-021ae275c688/scratchpad/probe78win.py
st() { echo "$* $(date -Is)" >> $J/status; }
until grep -q '^R6 L5_DONE' $J/status; do sleep 20; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
states() { for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o args= -p $p | awk '{print $1}' | xargs basename):$(awk '/^State/{print $2}' /proc/$p/status) "; done; }
{
echo "== (a) quiet (matchlock)"; touch $J/matchlock; sleep 10; echo "states: $(states)"; uptime
python3 $P
rm -f $J/matchlock
echo "== (b) gen5 running (baselock: bookgen stopped; gen4b held)"; touch $J/baselock; sleep 15; echo "states: $(states)"; uptime
python3 $P
rm -f $J/baselock
} > $JE/nps_check.txt 2>&1
st "EGDB8 nps_check done"
