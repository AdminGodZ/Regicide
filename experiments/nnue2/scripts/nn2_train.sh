#!/usr/bin/env bash
# NNUE round 2 (747ab51 trainer): nn2a (100 ep), nn2b (FT 512, 40 ep, 512 worktree), nn2c (lambda 0.5, 60 ep).
# Data: gen1..gen2b + gen3a snapshot. CPU-light (1 torch thread, 1 loader thread) so the baseline load stays <= 22 threads.
N2=~/regicide-nnue2-work
D=~/regicide-data/train
SNAP=$(ls $D/gen3a_partial_*.bin | head -n 1)
DATA="--data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --data $D/gen2a_n10k.bin --data $D/gen2b_n10k.bin --data $SNAP"
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
export OMP_NUM_THREADS=1
PY=~/regicide-nnue/.venv/bin/python
mkdir -p $N2/runs
run() {  # run NAME SRCDIR EPOCHS LAMBDA
  grep -q '^wrote ' $N2/runs/$1.log 2>/dev/null && return 0
  echo "start $1 ($2, epochs $3, lambda $4) $(date -Is)" >> $N2/status
  (cd $2 && $PY tools/nnue/train.py $DATA --out $N2/runs/$1 --epochs $3 --epoch-size 100000000 --batch 16384 \
     --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda $4 --qat-epochs 2 --compile --threads 1 --loader-threads 1 \
     >> $N2/runs/$1.log 2>&1)
  echo "end $1 rc=$? $(date -Is)" >> $N2/status
}
run nn2a ~/regicide-nnue 100 0.7
run nn2b ~/regicide-nnue512 40 0.7
run nn2c ~/regicide-nnue 60 0.5
echo "NN2_TRAIN_DONE $(date -Is)" >> $N2/status
