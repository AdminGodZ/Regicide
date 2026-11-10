#!/usr/bin/env bash
source /mnt/c/regicide-match/jobs/truth/common.sh
D=~/regicide-data/train; B=~/regicide-truth/build
F=""; for f in gen1_n10k gen1b_n10k gen1c_n10k gen2a_n10k gen2b_n10k gen3a_n10k gen3b_n10k gen3c_n10k gen3d_n10k gen4a_n10k gen4b_n10k gen5a_n50k gen5c_n50k; do F="$F $D/$f.bin"; done
st "census start"
nice -n 10 $B/truth census --egdb ~/regicide-data/egdb $F > $J/census.txt 2> $J/census.err
st "census end rc=$?"
