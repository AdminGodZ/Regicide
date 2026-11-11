#!/usr/bin/env bash
# train.sh NAME ARGS...: n6c's recipe (v1 1024-32-32, 8 buckets, 40 x 100M, lr 1e-3 -> 1e-5, batch 16384, wd 0.01,
# 2 QAT epochs, K fitted) with the trainer of this branch; one GPU lane capped at 1 torch + 1 loader thread.
# Data, lambda and any override (--ft, --l1, ...) come from ARGS. Afterwards the export check (engine evaldata vs the
# python integer net AND vs the simulated integer net of training, tools/nnue/check_export.py). Only if it passes is the
# net copied to $W/NAME.nnue and the marker $W/NAME.ok written (match.sh refuses a net without it); exit status 1
# otherwise. EVAL_ENGINE: the regicide binary for the check (a REGICIDE_NNUE_MAX_FT=2048 build for wide nets).
source /mnt/c/regicide-match/jobs/truth/common.sh
set -o pipefail
W=~/regicide-truth-work; T=~/regicide-truth; E=${EVAL_ENGINE:-$T/build/regicide}; PY=~/regicide-nnue/.venv/bin/python
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
  rc=$?
  st "train $n end rc=$rc"
  [ $rc = 0 ] || exit 1
fi
last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1)
chk=$(cd $T && OMP_NUM_THREADS=1 $PY tools/nnue/check_export.py --engine $E --data ~/regicide-nnue5-work/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --net $W/runs/$n/check.nnue --n 100000 2>&1)
rc=$?
{ echo "== $n export check (100k positions of the shared validation set), final $(basename $last), rc=$rc"; echo "$chk" | tail -n 5; } | tee -a $W/logs/export_checks.txt >> $W/runs/$n.log
if [ $rc != 0 ]; then
  rm -f $W/$n.ok
  st "train $n EXPORT CHECK FAILED (rc=$rc): $(echo "$chk" | tail -n 2 | tr '\n' ' ')"
  exit 1
fi
cmp -s $W/runs/$n/check.nnue $last || { st "train $n: re-export differs from $(basename $last)"; exit 1; }
cp $last $W/$n.nnue
echo "$chk" | tail -n 5 > $W/$n.ok
st "train $n exported $W/$n.nnue: $(echo "$chk" | grep 'simulated int' ) $(echo "$chk" | tail -n 1)"
