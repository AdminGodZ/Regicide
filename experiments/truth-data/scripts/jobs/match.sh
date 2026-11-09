#!/usr/bin/env bash
# match.sh NAME [MOVETIME] [OPENINGS_ROUNDS]: NAME.nnue (A, set evalfile) vs the built-in n6c (B), one rtruth.so
# (this branch = master fcf6acb engine) for both, openings4 805 x 2, -c 10, hash 128, book off, 2-8 EGDB, under the
# timing lock with matchlock (background generators stopped).
source /mnt/c/regicide-match/jobs/truth/common.sh
W=~/regicide-truth-work; n=$1; mt=${2:-0.1}; tag=$n-vs-n6c; [ "$mt" != 0.1 ] && tag=$tag-$mt
mkdir -p $W/matches
st "match $tag queued"
with_timing selfplay_match $n $W/$n.nnue $W/matches/$tag $W/engines/rtruth.so $mt
st "match $tag end: $(grep '^Elo (pent' $W/matches/$tag/summary.txt)"
