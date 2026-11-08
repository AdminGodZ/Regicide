#!/usr/bin/env bash
# nn1_train.sh EPOCHS [extra]: NNUE nn1 on gen1+gen1b+gen1c+gen2a+gen2b (README command), RTX 5080.
N=~/regicide-nnue-work
D=~/regicide-data/train
E=${1:-40}; shift
mkdir -p $N/runs
cd ~/regicide-nnue
export PATH=$(echo "$PATH" | tr ":" "
" | grep -v "^/mnt/c" | paste -sd:)
echo "start nn1 train epochs $E $* $(date -Is)" >> $N/status
.venv/bin/python tools/nnue/train.py \
  --data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --data $D/gen2a_n10k.bin --data $D/gen2b_n10k.bin \
  --out $N/runs/nn1 --epochs $E --epoch-size 100000000 --batch 16384 \
  --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile "$@" >> $N/runs/nn1.log 2>&1
echo "end nn1 train rc=$? $(date -Is)" >> $N/status
grep -q '^wrote ' $N/runs/nn1.log && echo "NN1_TRAIN_DONE $(date -Is)" >> $N/status
