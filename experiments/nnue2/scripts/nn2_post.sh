#!/usr/bin/env bash
# After nn2 training: export checks, losses on one validation set (nn2a's: lambda 0.7, its K), pick best two,
# then (after the 747ab51 baseline) match each vs nn1 with datagen paused.
N2=~/regicide-nnue2-work
J=~/regicide-jobs/a44591e
R=~/regicide-nnue; R5=~/regicide-nnue512
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
export OMP_NUM_THREADS=1
PY=$R/.venv/bin/python
st() { echo "$* $(date -Is)" >> $N2/status; }
until grep -q '^NN2_TRAIN_DONE' $N2/status; do sleep 60; done
{
for n in nn2a nn2b nn2c; do
  src=$R; [ $n = nn2b ] && src=$R5
  last=$(ls $N2/runs/$n/net_e*.nnue | sort | tail -n 1)
  echo "== $n export check ($src, final $(basename $last))"
  (cd $src && $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-data/train/gen2b_n10k.bin \
     --checkpoint $N2/runs/$n/last.pt --net $N2/$n.nnue 2>&1 | tail -n 4) 
  cmp $N2/$n.nnue $last && echo "$n.nnue identical to $(basename $last)"
done
} > $N2/export_checks.txt 2>&1
{
echo "== losses on nn2a's validation set (lambda 0.7, K from nn2a config)"
$PY $J/compare_loss.py $N2/runs/nn2a/config.json $N2 $R/build/regicide nn1=builtin nn2a=$N2/nn2a.nnue nn2c=$N2/nn2c.nnue G2c_linear=$R/src/weights/eval.bin
$PY $J/compare_loss.py $N2/runs/nn2a/config.json $N2 $R5/build/regicide nn2b=$N2/nn2b.nnue
} > $N2/losses.txt 2>&1
for n in nn2a nn2b nn2c; do src=$R; [ $n = nn2b ] && src=$R5; echo "== $n check on 100k validation positions" >> $N2/export_checks.txt; (cd $src && $PY tools/nnue/check_export.py --engine build/regicide --data $N2/nnue_valid_1M.bin --checkpoint $N2/runs/$n/last.pt --n 100000 2>&1 | tail -n 4) >> $N2/export_checks.txt; done
cat $N2/losses.txt >> $N2/status
BEST=$(grep -E '^nn2[abc] ' $N2/losses.txt | sort -k2 -n | head -n 2 | awk '{print $1}' | tr '\n' ' ')
st "best two by loss: $BEST"
until grep -q '^ALL_DONE baseline' $J/match747/status; do sleep 60; done
mkdir -p $N2/engines $N2/matches
[ -e $N2/engines/egdb ] || ln -s ~/regicide-data/egdb $N2/engines/egdb
cp $R/build/libregicide_cb.so $N2/engines/r747ab51.so
cp $R5/build/libregicide_cb.so $N2/engines/r747ab51_ft512.so
for n in $BEST; do
  SO=$N2/engines/r747ab51.so; [ $n = nn2b ] && SO=$N2/engines/r747ab51_ft512.so
  OUT=$N2/matches/$n-vs-nn1
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && continue
  touch $J/matchlock
  DG=$(pgrep -f '[d]atagen-(bd2ef1d|747ab51)' | tr '\n' ' ')
  [ -n "$DG" ] && kill -STOP $DG && st "paused datagen $DG"
  sleep 2
  rm -rf $OUT
  st "start match $n vs nn1 (A $(basename $SO) + set evalfile $n.nnue, B r747ab51.so built-in nn1)"
  $R/build/tools/match -a $SO -b $N2/engines/r747ab51.so --name-a $n --name-b nn1 \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $N2/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$?"
  DG2=$(pgrep -f '[d]atagen-(bd2ef1d|747ab51)' | tr '\n' ' ')
  [ -n "$DG2" ] && kill -CONT $DG2 && st "resumed datagen $DG2"
  rm -f $J/matchlock
done
st "NN2_MATCHES_DONE"
