#!/usr/bin/env bash
# group_loss.sh OUT NAME=NET...: per-phase validation loss on n8bn's E2 files' held-out tails (E5 datagen paused meanwhile)
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; out=$1; shift
D=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do D="$D --data $O/${f}_n10k_e2.bin"; done; D="$D --data $O/gen5a_n50k_e2.bin"
N=""; for n in "$@"; do N="$N --net $n"; done
touch $LOCKS/yield_truth
OMP_NUM_THREADS=1 nice ~/regicide-nnue/.venv/bin/python ~/regicide-truth/experiments/truth-data/scripts/group_loss.py $D $N --per-group 100000 > $out 2>&1
pgrep -f "regicide-jobs/truth/run_cb.sh|jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
st "group loss done: $out"
