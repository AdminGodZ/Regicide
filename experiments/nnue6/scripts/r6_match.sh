#!/usr/bin/env bash
# NNUE round 6 matches: n6a, n6b, n6c vs n4c, after the TM series and before the deep book (takes the matchlock as soon
# as the TM series ends; the deep book chain waits for the lock). One rb88c02e.so for both sides (A: set evalfile,
# B: built-in n4c), 0.1 s/move, openings4.txt 805 x 2, -c 10, book off, 2-8 EGDB. Datagen paused (matchlock).
J=~/regicide-jobs/r4; W=~/regicide-nnue6-work; JT=~/regicide-jobs/tm
st() { echo "R6N $* $(date -Is)" | tee -a $J/status >> $W/status; }
until grep -q '^TM TM_ALL_DONE' $J/status; do sleep 2; done
touch $J/matchlock
until grep -q '^R6N R6N_SCORED' $J/status; do sleep 30; done
sleep 8
st "matchlock taken: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
mkdir -p $W/matches
for n in n6a n6b n6c; do
  OUT=$W/matches/$n-vs-n4c
  grep -q '^Elo (pent' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs n4c (rb88c02e.so: A set evalfile $n.nnue, B built-in n4c)"
  $JT/bin/match-b88c02e --cbhost $JT/bin/cbhost -a $JT/engines/rb88c02e.so -b $JT/engines/rb88c02e.so --name-a $n --name-b n4c \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
rm -f $J/matchlock
st "R6N_DONE"
