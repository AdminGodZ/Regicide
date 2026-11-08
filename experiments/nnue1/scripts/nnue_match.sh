#!/usr/bin/env bash
# nnue_match.sh NAME_A SO_A NAME_B SO_B [extra option-a args...]: 805 openings x 2, 0.1 s/move, -c 10, book off,
# threads 1, hash 128, 2-7 piece egdb (symlink next to the .so files). Match binary + cbhost from the 1a32912 build.
N=~/regicide-nnue-work
A=$1 SA=$2 B=$3 SB=$4; shift 4
OUT=$N/matches/$A-vs-$B
mkdir -p $N/matches
echo "start match $A vs $B $(date -Is) (datagen running: $(pgrep -c -f '[d]atagen'))" >> $N/status
rm -rf $OUT
~/regicide-nnue/build/tools/match -a $SA -b $SB --name-a $A --name-b $B \
  --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" "$@" \
  --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
  --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
echo "end match $A vs $B rc=$? $(date -Is) (datagen running: $(pgrep -c -f '[d]atagen'))" >> $N/status
grep -q 'Games: 1610 of 1610' $OUT/summary.txt && echo "MATCH_DONE $A-vs-$B" >> $N/status
