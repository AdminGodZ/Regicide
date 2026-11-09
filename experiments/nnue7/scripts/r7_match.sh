#!/usr/bin/env bash
# NNUE round 7 matches vs n6c, after the deep2 tests (CPU free): n6d (lambda 0.3, round 6b), n7a (lambda 0.4),
# n7b (+gen5c x3, 60 epochs). One rff0a3d2.so (master ff0a3d2, built-in n6c, empty book) for both sides,
# 0.1 s/move, openings4.txt 805 x 2, -c 10, book off, 2-8 EGDB, datagen paused (matchlock).
J=~/regicide-jobs/r4; W=~/regicide-nnue7-work; W6=~/regicide-nnue6-work
st() { echo "R7 $* $(date -Is)" | tee -a $J/status >> $W/status; }
until grep -q '^D2 D2_ALL_DONE' $J/status && grep -q '^R7 R7_SCORED' $J/status; do sleep 30; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
touch $J/matchlock; sleep 8
st "matches start: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
mkdir -p $W/matches
for n in n6d n7a n7b; do
  NET=$W/$n.nnue; [ $n = n6d ] && NET=$W6/n6d.nnue
  OUT=$W/matches/$n-vs-n6c
  grep -q '^Elo (pent' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs n6c (rff0a3d2.so: A set evalfile $(basename $NET), B built-in n6c)"
  $W6/bin/match-ff0a3d2 --cbhost $W6/bin/cbhost -a $W6/engines/rff0a3d2.so -b $W6/engines/rff0a3d2.so --name-a $n --name-b n6c \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $NET" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
rm -f $J/matchlock
st "R7_DONE"
