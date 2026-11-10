#!/usr/bin/env bash
# e5_gen.sh NAME BIN SEED THREADS: E5 data, appended to ~/regicide-data/truth/NAME.bin (restartable: datagen appends,
# and its random streams depend on the records already in the file, so a restart never replays written games).
# 8-piece database (games end and take their result at the first <= 8-piece position; nothing <= 8 is written),
# 50k nodes/move, proven positions written with the flag, half the games from real-game seeds (plies 6-30 of the
# match PDNs vs Cake/KingsRow/GuiNN under ~/regicide-match/results) + 2-4 random plies, half from random openings.
# BIN: datagen-tr5 (yield group: also stops while another agent computes) or datagen-tr6 (base budget).
# Fails if the binary cannot be copied (never runs a stale one); logs its checksum; passes datagen's exit status on.
source /mnt/c/regicide-match/jobs/truth/common.sh
NAME=$1; BIN=$2; SEED=$3; TH=$4
if ! cp ~/regicide-truth/build/datagen $J/bin/$BIN; then
  st "e5 $NAME NOT started: cannot copy datagen to $J/bin/$BIN"
  exit 1
fi
st "e5 $NAME start ($BIN sha256 $(sha256sum $J/bin/$BIN | cut -c1-16), branch $(git -C ~/regicide-truth rev-parse --short HEAD), seed $SEED, $TH threads)"
$J/bin/$BIN --games 5000000 --nodes 50000 --threads $TH --seed $SEED --egdb ~/regicide-data/egdb \
  --seeds ~/regicide-data/truth/seeds_real_6_30.txt --seed-frac 0.5 --seed-plies 2-4 \
  --out ~/regicide-data/truth/$NAME.bin >> $J/e5_$NAME.log 2>&1
rc=$?
st "e5 $NAME end rc=$rc"
exit $rc
