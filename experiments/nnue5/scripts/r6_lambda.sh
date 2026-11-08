#!/usr/bin/env bash
# lambda sweep, v1 FT 1024 (n4c's arch), all data (data2), 40 epochs, n4c settings except --lambda:
#   l5a 0.5, l5b 0.9, l5c 1.0. One GPU lane (1 torch + 1 loader thread), after v2g.
# Then exact export checks, losses on v2d's validation set with three targets (lambda 0.7 as usual, 1.0 = result
# only, 0.0 = search score only), and matches l5a and l5b vs n4c (one rv2.so, 0.1 s/move, 1610 games), never
# overlapping another match or the 8-piece baseline.
J=~/regicide-jobs/r4; W=~/regicide-nnue5-work; R=~/regicide-v2; PY=~/regicide-nnue/.venv/bin/python
st() { echo "R6 $* $(date -Is)" | tee -a $J/status >> $W/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
st "r6_lambda start pid $$"
until grep -q '^R5b V2G_DONE' $J/status; do sleep 30; done
DATA=""; for f in $W/data2/*.bin; do DATA="$DATA --data $f"; done
lam() { case $1 in l5a) echo 0.5;; l5b) echo 0.9;; l5c) echo 1.0;; esac; }
for n in l5a l5b l5c; do
  grep -q '^wrote ' $W/runs/$n.log 2>/dev/null && continue
  st "start $n (v1 1024/32/32/8, lambda $(lam $n), 40 epochs, data2)"
  (cd $R && OMP_NUM_THREADS=1 $PY tools/nnue/train.py $DATA --out $W/runs/$n --arch v1 --ft 1024 --l1 32 --l2 32 --buckets 8 \
     --epochs 40 --epoch-size 100000000 --batch 16384 --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda $(lam $n) \
     --qat-epochs 2 --compile --threads 1 --loader-threads 1 > $W/runs/$n.log 2>&1)
  st "end $n rc=$?"
done
export OMP_NUM_THREADS=4
{
for n in l5a l5b l5c; do
  last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1)
  echo "== $n (lambda $(lam $n)) export check, 100k validation positions (v2d set)"
  (cd $R && $PY tools/nnue/check_export.py --engine build/regicide --data $W/val2/nnue_valid_1M.bin \
     --checkpoint $W/runs/$n/last.pt --net $W/$n.nnue --n 100000 2>&1 | tail -n 4)
  cmp $W/$n.nnue $last && echo "$n.nnue identical to $(basename $last)"
done
} > $W/export_checks_l5.txt 2>&1
for t in 0.7 1.0 0.0; do
  python3 -c "import json,sys; c=json.load(open('$W/runs/v2d/config.json')); c['lam']=$t; json.dump(c,open('$W/val2/config_lam$t.json','w'))"
done
{
echo "== losses on v2d's validation set (K 106.65 from v2d config), one t3/nnue-v2 binary; three targets"
for t in 0.7 1.0 0.0; do
  echo "-- target lambda $t"
  $PY /mnt/c/regicide-match/jobs/r5/compare_loss.py $W/val2/config_lam$t.json $W/val2 $R/build/regicide n4c=builtin \
    v2e=$W/v2e.nnue l5a=$W/l5a.nnue l5b=$W/l5b.nnue l5c=$W/l5c.nnue
done
} > $W/losses_l5.txt 2>&1
cat $W/losses_l5.txt >> $W/status
st "L5_TRAIN_DONE"
for n in l5a l5b; do
  OUT=$W/matches/$n-vs-n4c
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && continue
  # never overlap another match or the 8-piece baseline (which has priority once the EGDB is verified)
  while [ -e $J/matchlock ] || [ -e $J/baselock ] || \
        { grep -q '^EGDB8 EGDB8_DONE' $J/status && ! grep -q '^EGDB8BASE_DONE' $J/status; }; do sleep 60; done
  touch $J/matchlock; sleep 8
  st "matchlock taken: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$p:$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
  rm -rf $OUT
  st "start match $n vs n4c (one rv2.so: A set evalfile $n.nnue, B built-in n4c)"
  $J/bin/match-v2 --cbhost $J/bin/cbhost-v2 -a $W/engines/rv2.so -b $W/engines/rv2.so --name-a $n --name-b n4c \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
  rm -f $J/matchlock
done
st "L5_DONE"
