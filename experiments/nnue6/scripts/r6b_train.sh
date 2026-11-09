#!/usr/bin/env bash
# NNUE round 6b: follow-ups to n6c's recipe (v1 FT 1024; all 10k data + gen5a, 40 x 100M, lr 1e-3 -> 1e-5), trainer of
# master b88c02e (as round 6), one GPU lane capped at 1 torch + 1 loader thread:
#   n6d: lambda 0.3, gen5a x3        n6e: lambda 0.5, gen5a x6
#   n6f: lambda 0.5, gen5a x3 + a whole-record prefix of gen5c (50k nodes, still being generated) x3
# Scored like round 6 (shared set and gen5a's held-out tail, targets lambda 0.7 / 0.5 / 1.0 / 0.0) with n4c and n6c.
J=~/regicide-jobs/r4; W=~/regicide-nnue6-work; T=~/regicide-match/builds/b88c02e-egdb8/src; E=~/regicide-match/builds/b88c02e-egdb8/build-linux/regicide
PY=~/regicide-nnue/.venv/bin/python; D=~/regicide-data/train
st() { echo "R6B $* $(date -Is)" | tee -a $J/status >> $W/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
st "r6b_train start pid $$"
COMMON="--arch v1 --ft 1024 --l1 32 --l2 32 --buckets 8 --batch 16384 --wd 0.01 --qat-epochs 2 --compile --threads 1 --loader-threads 1 --epochs 40 --epoch-size 100000000 --lr 1e-3 --lr-final 1e-5"
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $D/${f}_n10k.bin"; done
G5="--data $D/gen5a_n50k.bin"
P=$(ls $D/gen5c_partial_*.bin 2>/dev/null | head -n 1)
if [ -z "$P" ]; then
  S=$(stat -c %s $D/gen5c_n50k.bin); N=$((S / 16)); P=$D/gen5c_partial_$N.bin
  head -c $((N * 16)) $D/gen5c_n50k.bin > $P.tmp && mv $P.tmp $P
  st "gen5c snapshot $P ($N records of $S bytes, gen5c still running)"
fi
G5C="--data $P"
run() {  # run NAME ARGS...
  local n=$1; shift
  grep -q '^wrote ' $W/runs/$n.log 2>/dev/null && return 0
  st "start $n"
  (cd $T && OMP_NUM_THREADS=1 $PY tools/nnue/train.py --out $W/runs/$n $COMMON "$@" > $W/runs/$n.log 2>&1)
  st "end $n rc=$?"
}
run n6d $ALL $G5 $G5 $G5 --lambda 0.3
run n6e $ALL $G5 $G5 $G5 $G5 $G5 $G5 --lambda 0.5
run n6f $ALL $G5 $G5 $G5 $G5C $G5C $G5C --lambda 0.5
st "R6B_TRAIN_DONE"
export OMP_NUM_THREADS=4
{
for n in n6d n6e n6f; do
  last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1); cp $last $W/$n.nnue
  echo "== $n export check (100k positions of v2d's validation set), final $(basename $last)"
  (cd $T && $PY tools/nnue/check_export.py --engine $E --data ~/regicide-nnue5-work/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4)
done
} > $W/export_checks_6b.txt 2>&1
python3 - <<EOF
import json
for s in ('shared', 'gen5a'):
    c = json.load(open('$W/val/%s_lam0.7.json' % s)); c['lam'] = 0.5; json.dump(c, open('$W/val/%s_lam0.5.json' % s, 'w'))
EOF
CL=/mnt/c/regicide-match/jobs/r5/compare_loss.py
NETS="n4c=builtin n6c=$W/n6c.nnue n6d=$W/n6d.nnue n6e=$W/n6e.nnue n6f=$W/n6f.nnue"
{
for set in shared gen5a; do
  for lam in 0.7 0.5 1.0 0.0; do
    echo "-- $set validation set, target lambda $lam"
    $PY $CL $W/val/${set}_lam$lam.json $W/val/$set $E $NETS
  done
done
} > $W/losses_6b.txt 2>&1
cat $W/losses_6b.txt >> $W/status
# the two best by loss on gen5a's held-out tail at target lambda 0.5 (n6c's lambda)
awk '/^-- gen5a validation set, target lambda 0.5/{f=1; next} /^--/{f=0} f && /^n6[def] /' $W/losses_6b.txt | sort -k2 -n | head -n 2 | awk '{print $1}' | tr '\n' ' ' > $W/r6b_selected
st "R6B_SCORED selected for matches vs n6c: $(cat $W/r6b_selected) (best gen5a-tail loss at target lambda 0.5)"
