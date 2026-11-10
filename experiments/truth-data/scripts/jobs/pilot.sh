#!/usr/bin/env bash
# E0 pilot: 200k random records with 9-14 pieces from every training file, searched with the 8-piece database at
# 10k, 50k and 200k nodes (6 threads, registered with pausekeeper_truth via the '[t]ruth-tr' pattern).
source /mnt/c/regicide-match/jobs/truth/common.sh
D=~/regicide-data/train; mkdir -p $J/bin; cp ~/regicide-truth/build/truth $J/bin/truth-tr1
F=""; for f in gen1_n10k gen1b_n10k gen1c_n10k gen2a_n10k gen2b_n10k gen3a_n10k gen3b_n10k gen3c_n10k gen3d_n10k gen4a_n10k gen5a_n50k gen5c_n50k; do F="$F $D/$f.bin"; done
st "pilot start"
$J/bin/truth-tr1 pilot --egdb ~/regicide-data/egdb --n 200000 --min 9 --max 14 --nodes 10000,50000,200000 --threads 6 --seed 11 --out $J/pilot.csv $F
st "pilot end rc=$?"
