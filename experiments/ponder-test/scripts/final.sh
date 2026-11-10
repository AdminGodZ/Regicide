#!/usr/bin/env bash
A=/mnt/c/regicide-match/jobs/ponder/analyze.py
for r in a1 a2 b1 b2 c1 c2; do python3 $A "$r=pt-s2-$r-cake" | grep -v "first25\|^     hits    moves.*\|^     no hit" ; done
for o in cake kingsrow guinn; do python3 $A "con-$o=pt-s2-con-$o" | grep -v "first25"; done
echo ======== pooled
python3 $A "A=pt-s2-a1-cake,pt-s2-a2-cake" "B=pt-s2-b1-cake,pt-s2-b2-cake" "C=pt-s2-c1-cake,pt-s2-c2-cake" | grep -v first25
echo ======== summaries
for r in ~/regicide-match/results/pt-*; do echo "-- $(basename $r)"; grep -E "^A |ponder:|^B |Terminations|forfeit|FATAL" $r/summary.txt; done
echo ======== status
grep "PT start\|matchlock removed\|PT end" ~/regicide-jobs/ponder/status | cut -c1-200
grep -il "forfeit\|crash\|killed\|fatal" ~/regicide-jobs/ponder/pt-*.log
pgrep -af "ponder/" ; ls ~/regicide-jobs/r4/matchlock 2>&1
