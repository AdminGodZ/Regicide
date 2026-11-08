#!/usr/bin/env bash
# NNUE round 4: train n4a (FT768 40ep), n4b (FT768 80ep), n4c (FT1024 40ep), n4d (FT768 L1 64 40ep) on
# gen1..gen3c + a whole-record prefix of gen3d, two GPU lanes, CPU-capped (1 torch + 1 loader thread).
# Then export checks, losses (n4a's validation set) incl. nn3e and nn2b, nps bench, matches of the best two vs nn3e.
J=~/regicide-jobs/r4; W=~/regicide-nnue4-work; D=~/regicide-data/train
R=~/regicide-385
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/runs $W/engines $W/matches
st() { echo "$* $(date -Is)" | tee -a $J/status >> $W/status; }
tsrc() { case $1 in n4c) echo ~/regicide-385-t1024;; n4d) echo ~/regicide-385-t768l64;; *) echo ~/regicide-385-t768;; esac; }
ebld() { case $1 in nn2b) echo $R/build;; n4c|nn3c) echo $R/build-ft1024;; n4d) echo $R/build-ft768_l164;; *) echo $R/build-ft768;; esac; }
st "r4_chain start pid $$"
until grep -q '^R4_LINUX_BUILD_DONE' $J/status; do sleep 30; done
# gen3d: finished file, or a whole-record prefix snapshot
G3D=$D/gen3d_n10k.bin
if grep -q '^gen3d datagen end' ~/regicide-jobs/r3/status; then
  st "gen3d finished: $(stat -c %s $G3D) bytes"
else
  P=$(ls $D/gen3d_partial_*.bin 2>/dev/null | head -n 1)
  if [ -z "$P" ]; then
    S=$(stat -c %s $G3D); N=$((S / 16)); P=$D/gen3d_partial_$N.bin
    head -c $((N * 16)) $G3D > $P.tmp && mv $P.tmp $P
    st "gen3d still running: snapshot $P ($N records of $S bytes, rem $((S % 16)))"
  fi
  G3D=$P
fi
for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c; do
  [ $(( $(stat -c %s $D/${f}_n10k.bin) % 16 )) -eq 0 ] || st "WARNING ${f} size not a multiple of 16"
done
DATA=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c; do DATA="$DATA --data $D/${f}_n10k.bin"; done
DATA="$DATA --data $G3D"
echo "$DATA" > $W/data_args.txt
run() {  # run NAME EPOCHS
  grep -q '^wrote ' $W/runs/$1.log 2>/dev/null && return 0
  st "start $1 ($(tsrc $1), epochs $2)"
  (cd $(tsrc $1) && OMP_NUM_THREADS=1 $PY tools/nnue/train.py $DATA --out $W/runs/$1 --epochs $2 --epoch-size 100000000 --batch 16384 \
     --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile --threads 1 --loader-threads 1 >> $W/runs/$1.log 2>&1)
  st "end $1 rc=$?"
}
( run n4a 40; run n4c 40; run n4d 40 ) &
sleep 120
run n4b 80
wait
st "NN4_TRAIN_DONE"
NETS="n4a n4b n4c n4d"
export OMP_NUM_THREADS=4
{
for n in $NETS; do
  last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1)
  echo "== $n export check ($(tsrc $n) vs $(ebld $n), final $(basename $last))"
  (cd $(tsrc $n) && $PY tools/nnue/check_export.py --engine $(ebld $n)/regicide --data ~/regicide-nnue-work/slice_gen2b_200k.bin \
     --checkpoint $W/runs/$n/last.pt --net $W/$n.nnue 2>&1 | tail -n 4)
  cmp $W/$n.nnue $last && echo "$n.nnue identical to $(basename $last)"
done
} > $W/export_checks.txt 2>&1
cp ~/regicide-nnue3-work/nn3e.nnue $W/nn3e.nnue
cp ~/regicide-nnue2-work/nn2b.nnue $W/nn2b.nnue
{
echo "== losses on n4a's validation set (lambda 0.7, K from n4a config), engine integer eval (385611e builds)"
cmp $W/nn2b.nnue $R/src/weights/net.bin && echo "(nn2b.nnue identical to the 385611e built-in net)"
$PY $J/compare_loss.py $W/runs/n4a/config.json $W $R/build/regicide nn2b=builtin
$PY $J/compare_loss.py $W/runs/n4a/config.json $W $R/build-ft768/regicide nn3e=$W/nn3e.nnue n4a=$W/n4a.nnue n4b=$W/n4b.nnue
$PY $J/compare_loss.py $W/runs/n4a/config.json $W $R/build-ft1024/regicide n4c=$W/n4c.nnue
$PY $J/compare_loss.py $W/runs/n4a/config.json $W $R/build-ft768_l164/regicide n4d=$W/n4d.nnue
} > $W/losses.txt 2>&1
for n in $NETS; do
  echo "== $n check on 100k validation positions" >> $W/export_checks.txt
  (cd $(tsrc $n) && $PY tools/nnue/check_export.py --engine $(ebld $n)/regicide --data $W/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4) >> $W/export_checks.txt
done
cat $W/losses.txt >> $W/status
BEST=$(grep -E '^n4[abcd] ' $W/losses.txt | sort -k2 -n | head -n 2 | awk '{print $1}' | tr '\n' ' ')
st "selected for matches: $BEST"
[ -e $W/engines/egdb ] || ln -s ~/regicide-data/egdb $W/engines/egdb
cp $R/build/libregicide_cb.so $W/engines/r385_ft512.so
cp $R/build-ft768/libregicide_cb.so $W/engines/r385_ft768.so
cp $R/build-ft768/libregicide_cb.so $W/engines/r385_ft768_A.so
cp $R/build-ft1024/libregicide_cb.so $W/engines/r385_ft1024.so
cp $R/build-ft768_l164/libregicide_cb.so $W/engines/r385_ft768_l164.so
mkdir -p $J/bin; cp $R/build/tools/match $J/bin/match-385611e; cp $R/build/tools/cbhost $J/bin/cbhost
# wait for the round-3 agent's matches, then take the lock and pause datagen
while [ -e ~/regicide-jobs/r3/matchlock ]; do sleep 60; done
touch $J/matchlock ~/regicide-jobs/r3/matchlock
DG=$(pgrep -f '[d]atagen-' | tr '\n' ' ')
[ -n "$DG" ] && kill -STOP $DG && st "paused datagen $DG for bench + matches"
sleep 3
{
echo "bench nodes and nps, 385611e builds, datagen paused, no other jobs (3 runs each)"
for i in 1 2 3; do
  echo "ft512 nn2b(builtin)    $(printf 'bench\nquit\n' | $R/build/regicide | grep '^bench')"
  echo "ft768 nn3e             $(printf "set evalfile $W/nn3e.nnue\nbench\nquit\n" | $R/build-ft768/regicide | grep '^bench')"
  echo "ft768 n4a              $(printf "set evalfile $W/n4a.nnue\nbench\nquit\n" | $R/build-ft768/regicide | grep '^bench')"
  echo "ft1024 n4c             $(printf "set evalfile $W/n4c.nnue\nbench\nquit\n" | $R/build-ft1024/regicide | grep '^bench')"
  echo "ft768 l1 64 n4d        $(printf "set evalfile $W/n4d.nnue\nbench\nquit\n" | $R/build-ft768_l164/regicide | grep '^bench')"
done
} > $W/bench.txt 2>&1
st "bench done"
for n in $BEST; do
  case $n in n4c) SO=$W/engines/r385_ft1024.so;; n4d) SO=$W/engines/r385_ft768_l164.so;; *) SO=$W/engines/r385_ft768_A.so;; esac
  OUT=$W/matches/$n-vs-nn3e
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs nn3e (A $(basename $SO) + evalfile $n.nnue, B r385_ft768.so + evalfile nn3e.nnue)"
  $J/bin/match-385611e -a $SO -b $W/engines/r385_ft768.so --name-a $n --name-b nn3e \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" --option-b "set evalfile $W/nn3e.nnue" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
[ -n "$DG" ] && kill -CONT $DG && st "resumed datagen $DG"
rm -f $J/matchlock ~/regicide-jobs/r3/matchlock
st "NN4_DONE"
