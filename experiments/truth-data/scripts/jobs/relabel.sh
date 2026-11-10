#!/usr/bin/env bash
# E2: game results := database value of each game's first record with <= 8 pieces (truth relabel), every file.
source /mnt/c/regicide-match/jobs/truth/common.sh
D=~/regicide-data/train; O=~/regicide-data/truth; mkdir -p $O $J/bin; cp ~/regicide-truth/build/truth $J/bin/truth-tr1
st "relabel start"
for f in gen5c_n50k gen5a_n50k gen4a_n10k gen3d_n10k gen3c_n10k gen3b_n10k gen3a_n10k gen2b_n10k gen2a_n10k gen1c_n10k gen1b_n10k gen1_n10k; do
  [ -s $O/${f}_e2.bin ] && [ $(stat -c %s $O/${f}_e2.bin) = $(stat -c %s $D/$f.bin) ] && continue
  $J/bin/truth-tr1 relabel --egdb ~/regicide-data/egdb --in $D/$f.bin --out $O/${f}_e2.bin.tmp >> $J/relabel.txt && mv $O/${f}_e2.bin.tmp $O/${f}_e2.bin
done
st "relabel end"
