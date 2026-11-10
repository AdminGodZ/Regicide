#!/usr/bin/env bash
# train.sh NAME ARGS...: n6c's recipe (v1 1024-32-32, 8 buckets, 40 x 100M, lr 1e-3 -> 1e-5, batch 16384, wd 0.01,
# 2 QAT epochs, K fitted) with the trainer of this branch; one GPU lane capped at 1 torch + 1 loader thread.
# Data and lambda come from ARGS. Afterwards: export check (engine evaldata vs python) and copy to $W/NAME.nnue.
source /mnt/c/regicide-match/jobs/truth/common.sh
W=~/regicide-truth-work; T=~/regicide-truth; E=$T/build/regicide; PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/runs $W/logs
n=$1; shift
COMMON="--arch v1 --ft 1024 --l1 32 --l2 32 --buckets 8 --batch 16384 --wd 0.01 --qat-epochs 2 --compile --threads 1 --loader-threads 1 --epochs 40 --epoch-size 100000000 --lr 1e-3 --lr-final 1e-5"
# one GPU run at a time (my training jobs queue on this lock)
exec 8>$J/gpu.lock; flock 8
# never train while one of my self-play matches (match-ff0a3d2) is running
while pgrep -f "[m]atch-ff0a3d2" > /dev/null; do sleep 20; done
if ! grep -q '^wrote ' $W/runs/$n.log 2>/dev/null; then
  st "train $n start: $*"
  (cd $T && OMP_NUM_THREADS=1 $PY tools/nnue/train.py --out $W/runs/$n $COMMON "$@" > $W/runs/$n.log 2>&1)
  st "train $n end rc=$?"
fi
last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1); cp $last $W/$n.nnue
{ echo "== $n export check (100k positions of the shared validation set), final $(basename $last)"
  (cd $T && OMP_NUM_THREADS=1 $PY tools/nnue/check_export.py --engine $E --data ~/regicide-nnue5-work/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4); } >> $W/logs/export_checks.txt
st "train $n exported $W/$n.nnue: $(tail -n 1 $W/logs/export_checks.txt)"
