#!/usr/bin/env bash
# e5_gen.sh NAME BIN SEED THREADS: E5 data, appended to ~/regicide-data/truth/NAME.bin (restartable: datagen appends).
# 8-piece database (games end and take their result at the first <= 8-piece position; nothing <= 8 is written),
# 50k nodes/move, proven positions written with the flag, half the games from real-game seeds (plies 6-30 of the
# match PDNs vs Cake/KingsRow/GuiNN under ~/regicide-match/results) + 2-4 random plies, half from random openings.
# BIN: datagen-tr5 (yield group: also stops while another agent computes) or datagen-tr6 (base budget).
source /mnt/c/regicide-match/jobs/truth/common.sh
NAME=$1; BIN=$2; SEED=$3; TH=$4
cp ~/regicide-truth/build/datagen $J/bin/$BIN 2>/dev/null
st "e5 $NAME start ($BIN, seed $SEED, $TH threads)"
$J/bin/$BIN --games 5000000 --nodes 50000 --threads $TH --seed $SEED --egdb ~/regicide-data/egdb \
  --seeds ~/regicide-data/truth/seeds_real_6_30.txt --seed-frac 0.5 --seed-plies 2-4 \
  --out ~/regicide-data/truth/$NAME.bin >> $J/e5_$NAME.log 2>&1
st "e5 $NAME end rc=$?"
