#!/usr/bin/env bash
# Round-6b matches: the two selected nets vs n6c, right after the T6 confirmation runs and before the deep book
# (takes the matchlock as soon as the last T6 run ends; the deep book chain waits for it). One rff0a3d2.so (master
# ff0a3d2, built-in n6c) for both sides (A: set evalfile, B: built-in), 0.1 s/move, openings4.txt 805 x 2, -c 10,
# book off, 2-8 EGDB, datagen paused.
J=~/regicide-jobs/r4; W=~/regicide-nnue6-work
st() { echo "R6B $* $(date -Is)" | tee -a $J/status >> $W/status; }
until grep -q '^BA end tm-T6 guinn' $J/status; do sleep 1; done
touch $J/matchlock
st "matchlock taken after the T6 runs"
until grep -q '^R6B R6B_SCORED' $J/status; do sleep 30; done
sleep 8
st "match start: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
for n in $(cat $W/r6b_selected); do
  OUT=$W/matches/$n-vs-n6c
  grep -q '^Elo (pent' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs n6c (rff0a3d2.so: A set evalfile $n.nnue, B built-in n6c)"
  $W/bin/match-ff0a3d2 --cbhost $W/bin/cbhost -a $W/engines/rff0a3d2.so -b $W/engines/rff0a3d2.so --name-a $n --name-b n6c \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
rm -f $J/matchlock
st "R6B_DONE"
