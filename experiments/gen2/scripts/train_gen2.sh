#!/usr/bin/env bash
# gen2 candidates: a44591e trainer on gen1+gen1b+gen1c+gen2a, lambda 0.7, batch 65536, coupled L2 off, wd 0, 12 threads.
J=~/regicide-jobs/a44591e
SRC=~/regicide-gen2
W=~/regicide-gen2-work/train
D=~/regicide-data/train
mkdir -p $W
TR=$J/bin/train-a44591e
[ -x $TR ] || cp $SRC/build/train $TR
DATA="--data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --data $D/gen2a_n10k.bin"
st() { echo "$* $(date -Is)" >> $W/status; }
st "train start commit $(git -C $SRC rev-parse --short HEAD)"
run() {  # run <name> <lr> <epochs> [extra]
  grep -q '^wrote' $W/train_$1.log 2>/dev/null && return 0
  st "start $1"
  $TR $DATA --out $W/w_$1.bin --epochs $3 --lr $2 --batch 65536 --lambda 0.7 --l2 0 --l2-table 0 --wd 0 --wd-table 0 \
    --threads 12 "${@:4}" > $W/train_$1.log 2>&1
  st "end $1 rc=$?"
}
run builtin_e0 0.03 0 --init $SRC/experiments/gen1full/wd/w_lr0.03_wd0.bin
run G2a 0.03 8
run G2b 0.1 8
run G2c 0.1 16
run G2d 0.3 8
st "TRAIN_DONE"
