#!/usr/bin/env bash
# NNUE round 7 (vs n6c): v1 FT 1024, n6c's recipe (all 10k data + gen5a x3, 40 x 100M, lr 1e-3 -> 1e-5, lambda 0.5),
# trainer of master b88c02e (as rounds 6/6b), one GPU lane capped at 1 torch + 1 loader thread.
#   n7a: lambda 0.4  (n6d = the same recipe at lambda 0.3 exists from round 6b and is reused as the 0.3 candidate)
#   n7b: + the gen5c done so far (whole-record snapshot) x3, 60 epochs (1.5x)
# Scored like round 6 (shared set and gen5a's held-out tail; targets lambda 0.7 / 0.5 / 1.0 / 0.0) with n4c, n6c, n6d.
J=~/regicide-jobs/r4; W=~/regicide-nnue7-work; W6=~/regicide-nnue6-work
T=~/regicide-match/builds/b88c02e-egdb8/src; E=~/regicide-match/builds/b88c02e-egdb8/build-linux/regicide
PY=~/regicide-nnue/.venv/bin/python; D=~/regicide-data/train
st() { echo "R7 $* $(date -Is)" | tee -a $J/status >> $W/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/runs $W/val
st "r7_train start pid $$"
COMMON="--arch v1 --ft 1024 --l1 32 --l2 32 --buckets 8 --batch 16384 --wd 0.01 --qat-epochs 2 --compile --threads 1 --loader-threads 1 --epoch-size 100000000 --lr 1e-3 --lr-final 1e-5"
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $D/${f}_n10k.bin"; done
G5="--data $D/gen5a_n50k.bin"
S=$(stat -c %s $D/gen5c_n50k.bin); N=$((S / 16)); P=$D/gen5c_partial_$N.bin
[ -e $P ] || { head -c $((N * 16)) $D/gen5c_n50k.bin > $P.tmp && mv $P.tmp $P; }
st "gen5c snapshot $P ($N records; gen5c still running)"
G5C="--data $P"
run() { local n=$1; shift
  grep -q '^wrote ' $W/runs/$n.log 2>/dev/null && return 0
  st "start $n"
  (cd $T && OMP_NUM_THREADS=1 $PY tools/nnue/train.py --out $W/runs/$n $COMMON "$@" > $W/runs/$n.log 2>&1)
  st "end $n rc=$?"
}
run n7a $ALL $G5 $G5 $G5 --epochs 40 --lambda 0.4
run n7b $ALL $G5 $G5 $G5 $G5C $G5C $G5C --epochs 60 --lambda 0.5
st "R7_TRAIN_DONE"
export OMP_NUM_THREADS=4
{ for n in n7a n7b; do
    last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1); cp $last $W/$n.nnue
    echo "== $n export check (100k positions of v2d's validation set), final $(basename $last)"
    (cd $T && $PY tools/nnue/check_export.py --engine $E --data ~/regicide-nnue5-work/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4)
  done; } > $W/export_checks.txt 2>&1
CL=/mnt/c/regicide-match/jobs/r5/compare_loss.py
NETS="n4c=builtin n6c=$W6/n6c.nnue n6d=$W6/n6d.nnue n7a=$W/n7a.nnue n7b=$W/n7b.nnue"
{ for set in shared gen5a; do for lam in 0.7 0.5 1.0 0.0; do
    echo "-- $set validation set, target lambda $lam"
    mkdir -p $W/val/$set; $PY $CL $W6/val/${set}_lam$lam.json $W/val/$set $E $NETS
  done; done; } > $W/losses.txt 2>&1
cat $W/losses.txt >> $W/status
st "R7_SCORED"
