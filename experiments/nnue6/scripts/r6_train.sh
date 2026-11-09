#!/usr/bin/env bash
# NNUE round 6: do 50k-node labels (gen5a) beat 10k ones? v1 FT 1024 (n4c's arch), trainer of master b88c02e,
# one GPU lane capped at 1 torch + 1 loader thread.
#   n6a: fine-tune n4c (round-4 last.pt; epoch/step reset, K refitted) on gen5a only: lr 2e-4 -> 1e-5, 15 x 50M, lambda 0.7
#   n6b: from scratch on all finished data (gen1..gen4a) + gen5a x3 (repeated --data), 40 x 100M, n4c settings
#   n6c: as n6b with lambda 0.5
# Then exact export checks and losses on (1) v2d's shared validation set and (2) gen5a's held-out 2% tail, each with
# three targets (lambda 0.7 / 1.0 = result / 0.0 = search score), for n4c, v2e, l5a, n6a, n6b, n6c.
J=~/regicide-jobs/r4; W=~/regicide-nnue6-work; T=~/regicide-match/builds/b88c02e-egdb8/src; E=~/regicide-match/builds/b88c02e-egdb8/build-linux/regicide
PY=~/regicide-nnue/.venv/bin/python; D=~/regicide-data/train; W5=~/regicide-nnue5-work
st() { echo "R6N $* $(date -Is)" | tee -a $J/status >> $W/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/runs $W/val
st "r6_train start pid $$"
COMMON="--arch v1 --ft 1024 --l1 32 --l2 32 --buckets 8 --batch 16384 --wd 0.01 --qat-epochs 2 --compile --threads 1 --loader-threads 1"
# n6a: fine-tune start checkpoint
if [ ! -f $W/runs/n6a_init.pt ]; then
  (cd $T/tools/nnue && OMP_NUM_THREADS=1 $PY -c "
import torch
ck = torch.load('$HOME/regicide-nnue4-work/runs/n4c/last.pt', map_location='cpu')
print('n4c checkpoint epoch', ck['epoch'], 'K', ck['K'])
ck['epoch'] = 0; ck['step'] = 0; ck['K'] = 0.0
torch.save(ck, '$W/runs/n6a_init.pt')") >> $W/status 2>&1
fi
run() {  # run NAME ARGS...
  local n=$1; shift
  grep -q '^wrote ' $W/runs/$n.log 2>/dev/null && return 0
  st "start $n: $*"
  (cd $T && OMP_NUM_THREADS=1 $PY tools/nnue/train.py --out $W/runs/$n $COMMON "$@" > $W/runs/$n.log 2>&1)
  st "end $n rc=$?"
}
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $D/${f}_n10k.bin"; done
G5="--data $D/gen5a_n50k.bin"
run n6a $G5 --resume $W/runs/n6a_init.pt --epochs 15 --epoch-size 50000000 --lr 2e-4 --lr-final 1e-5 --lambda 0.7
run n6b $ALL $G5 $G5 $G5 --epochs 40 --epoch-size 100000000 --lr 1e-3 --lr-final 1e-5 --lambda 0.7
run n6c $ALL $G5 $G5 $G5 --epochs 40 --epoch-size 100000000 --lr 1e-3 --lr-final 1e-5 --lambda 0.5
st "R6N_TRAIN_DONE"
export OMP_NUM_THREADS=4
{
for n in n6a n6b n6c; do
  last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1); cp $last $W/$n.nnue
  echo "== $n export check (100k positions of v2d's validation set), final $(basename $last)"
  (cd $T && $PY tools/nnue/check_export.py --engine $E --data $W5/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4)
done
} > $W/export_checks.txt 2>&1
# validation set 2: gen5a's held-out tail (valid_frac 0.02 of the file), K and seed of v2d's config
python3 - <<EOF
import json
c = json.load(open('$W5/runs/v2d/config.json'))
for lam in (0.7, 1.0, 0.0):
    c1 = dict(c, lam=lam); json.dump(c1, open('$W/val/shared_lam%s.json' % lam, 'w'))
    c2 = dict(c, lam=lam, data=['$D/gen5a_n50k.bin']); json.dump(c2, open('$W/val/gen5a_lam%s.json' % lam, 'w'))
EOF
mkdir -p $W/val/shared $W/val/gen5a
CL=/mnt/c/regicide-match/jobs/r5/compare_loss.py
NETS="n4c=builtin v2e=$W5/v2e.nnue l5a=$W5/l5a.nnue n6a=$W/n6a.nnue n6b=$W/n6b.nnue n6c=$W/n6c.nnue"
{
for set in shared gen5a; do
  for lam in 0.7 1.0 0.0; do
    echo "-- $set validation set, target lambda $lam"
    $PY $CL $W/val/${set}_lam$lam.json $W/val/$set $E $NETS
  done
done
} > $W/losses.txt 2>&1
cat $W/losses.txt >> $W/status
st "R6N_SCORED"
