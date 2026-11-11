#!/usr/bin/env bash
# e5_gen.sh NAME BIN SEED THREADS: E5 data, appended to ~/regicide-data/truth/NAME.bin (restartable: datagen appends,
# and its random streams depend on the records already in the file, so a restart never replays written games).
# 8-piece database (games end and take their result at the first <= 8-piece position; nothing <= 8 is written),
# 50k nodes/move, proven positions written with the flag, half the games from real-game seeds (plies 6-30 of the
# match PDNs vs Cake/KingsRow/GuiNN under ~/regicide-match/results) + 2-4 random plies, half from random openings.
# BIN: datagen-tr5 (yield group: also stops while another agent computes) or datagen-tr6 (base budget).
# Fails if the binary cannot be copied (never runs a stale one); logs its checksum; passes datagen's exit status on.
# PARITY (0|1, optional) and OTHER (optional: the other generator's file name): the starts already in this file and in
# OTHER are never played again (--avoid), and this generator plays only starts of hash parity PARITY.
source /mnt/c/regicide-match/jobs/truth/common.sh
NAME=$1; BIN=$2; SEED=$3; TH=$4; PARITY=${5:-}; OTHER=${6:-}
EXTRA="--avoid $HOME/regicide-data/truth/$NAME.bin"
[ -n "$OTHER" ] && EXTRA="$EXTRA --avoid $HOME/regicide-data/truth/$OTHER.bin"
[ -n "$PARITY" ] && EXTRA="$EXTRA --start-parity $PARITY"
# RANDOM_PLIES=A-B (environment, optional): random plies of the random-opening half (datagen's default 4-12)
[ -n "${RANDOM_PLIES:-}" ] && EXTRA="$EXTRA --random-min ${RANDOM_PLIES%-*} --random-max ${RANDOM_PLIES#*-}"
if [ -n "${EXPECT_SHA:-}" ]; then
  # use the binary already in $J/bin, only if its sha256 starts with EXPECT_SHA (no copy)
  sha=$(sha256sum $J/bin/$BIN | cut -c1-64)
  case $sha in
    $EXPECT_SHA*) ;;
    *) st "e5 $NAME NOT started: $J/bin/$BIN has sha256 ${sha:0:16}, expected $EXPECT_SHA"; exit 1 ;;
  esac
elif ! cp ~/regicide-truth/build/datagen $J/bin/$BIN; then
  st "e5 $NAME NOT started: cannot copy datagen to $J/bin/$BIN"
  exit 1
fi
st "e5 $NAME start ($BIN sha256 $(sha256sum $J/bin/$BIN | cut -c1-16), branch $(git -C ~/regicide-truth rev-parse --short HEAD), seed $SEED, $TH threads, parity ${PARITY:-any}, avoid $NAME${OTHER:+ + $OTHER}, random plies ${RANDOM_PLIES:-4-12}, from record $(( $(stat -c %s ~/regicide-data/truth/$NAME.bin) / 16 )))"
$J/bin/$BIN --games 5000000 --nodes 50000 --threads $TH --seed $SEED --egdb ~/regicide-data/egdb $EXTRA \
  --seeds ~/regicide-data/truth/seeds_real_6_30.txt --seed-frac 0.5 --seed-plies 2-4 \
  --out ~/regicide-data/truth/$NAME.bin >> $J/e5_$NAME.log 2>&1
rc=$?
st "e5 $NAME end rc=$rc"
exit $rc
