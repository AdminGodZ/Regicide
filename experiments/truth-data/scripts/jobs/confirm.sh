#!/usr/bin/env bash
# confirm.sh NAME [MOVETIME]: NAME.nnue vs built-in n6c at MOVETIME (default 0.5 s/move) over openings4's 805 pairs,
# split into 4 chunks (~200 pairs, ~14 min each at 0.5 s) so that each run holds the timing lock < 20 min; pooled with
# pool_dirs.py. Same engine and settings as match.sh.
source /mnt/c/regicide-match/jobs/truth/common.sh
W=~/regicide-truth-work; n=$1; mt=${2:-0.5}; C=$W/chunks; mkdir -p $C $W/matches
[ -s $C/op4_a ] || split -n l/4 -a 1 ~/regicide-gen2-work/openings4.txt $C/op4_
for k in a b c d; do
  tag=$n-vs-n6c-$mt-$k
  st "confirm $tag queued"
  with_timing selfplay_match $n $W/$n.nnue $W/matches/$tag $W/engines/rtruth.so $mt n6c "" $C/op4_$k
  st "confirm $tag end: $(grep '^Elo (pent' $W/matches/$tag/summary.txt)"
done
st "confirm $n $mt pooled: $(~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/pool_dirs.py $W/matches/$n-vs-n6c-$mt-? | tail -n 1)"
