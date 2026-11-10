#!/usr/bin/env bash
# Round-7 matches vs n6c (n6d, n7a, n7b), the r7_match.sh settings, each under the timing lock + matchlock.
source /mnt/c/regicide-match/jobs/truth/common.sh
W=~/regicide-nnue7-work; W6=~/regicide-nnue6-work; mkdir -p $W/matches
for n in n7b n7a n6d; do
  NET=$W/$n.nnue; [ $n = n6d ] && NET=$W6/n6d.nnue
  st "r7 match $n vs n6c queued"
  with_timing selfplay_match $n $NET $W/matches/$n-vs-n6c $W6/engines/rff0a3d2.so 0.1
  st "r7 end $n: $(grep '^Elo (pent' $W/matches/$n-vs-n6c/summary.txt)"
done
st "R7_DONE (truth agent)"
