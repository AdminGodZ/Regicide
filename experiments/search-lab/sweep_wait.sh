#!/bin/bash
# sweep_wait.sh [SECONDS]: waits (file reads only) for a DONE/FAILED line of the current runner run that is not in
# posted.txt; posts a DONE through post.sh (which refuses bad rows), reports a FAILED, and records it.
J=~/regicide-jobs/searchlab; touch $J/posted.txt
pending() { tac $J/runner.log | awk '/^RUNNER START/{exit} {print}' | tac | /usr/bin/grep -E '^(DONE|FAILED) ' | awk '{print $1, $2}' | /usr/bin/grep -vxFf $J/posted.txt | head -1; }
export -f pending; export J
timeout ${1:-590} bash -c 'until [ -n "$(pending)" ]; do sleep 30; done'
p=$(pending)
if [ -z "$p" ]; then
  cur=$(tac $J/runner.log | awk '/^RUNNER START/{exit} {print}' | /usr/bin/grep -m1 '^BEGIN' | awk '{print $2}')
  echo "running: $cur $(/usr/bin/grep -a games $J/logs/$cur.log 2>/dev/null | tail -1)"; exit 0
fi
set -- $p
if [ "$1" = DONE ]; then
  v=${2#lmr_}; v=${v%%_*}; label="LMR ${v/base/lmr_base=}"; label=${label/mult/lmr_mult=}
  case $2 in *_conf600) label="$label confirmation, 600 ms, other half (--skip 800)" ;; *) label="$label (200 ms)" ;; esac
  $J/post.sh "$2" "$label" "LMR sweep: $2" && echo "$p" >> $J/posted.txt
else
  echo "FAILED test: $2 (log kept, see runner.log)"; echo "$p" >> $J/posted.txt
fi
