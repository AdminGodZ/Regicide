#!/usr/bin/env bash
# Switch the book/avoid tests to the master 769ef0b DLL:
#  1. wait until the running 65f7143 run (avoid kingsrow) ends, stop the old chain (its next run is discarded),
#  2. EGDB speed check 2 with the 769ef0b DLL (matchlock: generators stopped),
#  3. choose the EGDB setting for the tests: "set dbmbytes 12288" if its 8-process nps beats the default by > 2%,
#     else the default; written to ~/regicide-jobs/ba/egdb_setting, then SWITCH769_DONE.
J=~/regicide-jobs/r4; JB=~/regicide-jobs/ba; JE=~/regicide-jobs/egdb8
st() { echo "$* $(date -Is)" >> $J/status; }
until grep -q '^BA end avoid kingsrow' $J/status; do sleep 5; done
for p in $(pgrep -f 'bash /mnt/c/regicide-match/jobs/ba/ba_chain.sh'); do kill $p; done
sleep 3
for p in $(pgrep -f 'match-747ab51 .*results/ba-'); do kill $p; done
sleep 5
for d in ~/regicide-match/results/ba-off-cake; do [ -d $d ] && mv $d $JB/interrupted-$(basename $d) && mv $J/$(basename $d).log $JB/interrupted-$(basename $d).log; done
st "BA 65f7143 chain stopped after avoid kingsrow (switch to 769ef0b); runs kept: ba-off-kingsrow, ba-avoid-kingsrow"
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 10; done
touch $J/matchlock; sleep 8
st "EGDB8 speed check 2 (769ef0b) start"
{ date -Is; for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done; echo
  python3 /mnt/c/regicide-match/jobs/egdb8/speed_check2.py; date -Is; } > $JE/speed_check2.txt 2>&1
rm -f $J/matchlock
A=$(grep -E '^round [12] 8-piece 4096 MB +x8' $JE/speed_check2.txt | sed -E 's/.* all ([0-9.]+)M.*/\1/' | awk '{s+=$1} END {print s/NR}')
B=$(grep -E '^round [12] 8-piece 12288 MB +x8' $JE/speed_check2.txt | sed -E 's/.* all ([0-9.]+)M.*/\1/' | awk '{s+=$1} END {print s/NR}')
if awk "BEGIN{exit !($B > $A * 1.02)}"; then echo "set dbmbytes 12288" > $JB/egdb_setting; else : > $JB/egdb_setting; fi
st "EGDB8 speed check 2 done: 8-piece x8 nps default ${A}M, dbmbytes 12288 ${B}M -> setting '$(cat $JB/egdb_setting)'"
st "SWITCH769_DONE"
