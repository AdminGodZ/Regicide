#!/usr/bin/env bash
# NNUE round 5 (v2 architecture, t3/nnue-v2): train v2a / v2b / v2c with experiments/nnue_v2/scripts/train_v2.sh on
# gen1..gen3d (full) + a whole-record gen4a prefix; one GPU lane while the 2 s KingsRow run is going, two after.
# Then losses on one shared validation set (incl. n4c), nps bench, matches of the best two vs n4c.
J=~/regicide-jobs/r4; W=~/regicide-nnue5-work; D=~/regicide-data/train; R=~/regicide-v2
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/data $W/claims $W/engines $W/matches
st() { echo "R5 $* $(date -Is)" | tee -a $J/status >> $W/status; }
st "r5_chain start pid $$"
until grep -q '^R5_BUILD_DONE' $J/status; do sleep 30; done
# data: finished files (no snapshots, no gen1b_snap duplicate) + gen4a prefix snapshot
if [ ! -e $W/data/.done ]; then
  for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d; do ln -sf $D/${f}_n10k.bin $W/data/${f}_n10k.bin; done
  if grep -q '^gen4a datagen end' $J/status; then
    ln -sf $D/gen4a_n10k.bin $W/data/gen4a_n10k.bin; st "gen4a finished: $(stat -c %s $D/gen4a_n10k.bin) bytes"
  else
    S=$(stat -c %s $D/gen4a_n10k.bin); N=$((S / 16)); P=$D/gen4a_partial_$N.bin
    head -c $((N * 16)) $D/gen4a_n10k.bin > $P.tmp && mv $P.tmp $P
    ln -sf $P $W/data/gen4a_prefix_$N.bin
    st "gen4a still running: snapshot $P ($N records of $S bytes)"
  fi
  touch $W/data/.done
fi
ls -l $W/data > $W/data_files.txt
args() { case $1 in v2a) echo "--arch v2 --ft 512 --l1 32 --l2 32 --buckets 8";; v2b) echo "--arch v2 --ft 1024 --l1 16 --l2 32 --buckets 8";; v2c) echo "--arch v2 --ft 512 --buckets 1";; esac; }
lane() {  # lane N: claim and train the next unclaimed net
  for n in v2a v2b v2c; do
    mkdir $W/claims/$n 2>/dev/null || continue
    if grep -q '^wrote ' $W/runs/$n.log 2>/dev/null && [ -f $W/$n.nnue ]; then continue; fi
    st "lane $1 start $n ($(args $n))"
    (cd $R && DATA_DIR=$W/data WORK=$W PY=$PY bash experiments/nnue_v2/scripts/train_v2.sh $n $(args $n) > $W/$n.train_v2.out 2>&1)
    st "lane $1 end $n rc=$? $(grep -m1 'max |diff|' $W/runs/$n.check.txt) $(tail -n 1 $W/runs/$n.check.txt)"
  done
}
lane 1 &
L1=$!
( until grep -q 'BASELINE end eb1037c-n4c kingsrow-2s' $J/status; do sleep 60; done; st "2 s run over: lane 2 starts"; lane 2 ) &
L2=$!
wait $L1 $L2
st "R5_TRAIN_DONE"
NETS="v2a v2b v2c"
export OMP_NUM_THREADS=4
{
for n in $NETS; do
  last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1)
  echo "== $n ($(args $n)) export check on 100k of the last data file (train_v2.sh)"; cat $W/runs/$n.check.txt
  cmp $W/$n.nnue $last && echo "$n.nnue identical to $(basename $last)"
  echo "== $n check on the 200k gen2b slice"
  (cd $R && $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-nnue-work/slice_gen2b_200k.bin \
     --checkpoint $W/runs/$n/last.pt 2>&1 | tail -n 4)
done
} > $W/export_checks.txt 2>&1
{
echo "== losses on v2a's validation set (lambda 0.7, K from v2a config), engine integer eval (t3/nnue-v2 build, one binary)"
cmp $R/src/weights/net.bin ~/regicide-nnue4-work/n4c.nnue && echo "(built-in net.bin identical to n4c.nnue)"
$PY /mnt/c/regicide-match/jobs/r5/compare_loss.py $W/runs/v2a/config.json $W $R/build/regicide n4c=builtin v2a=$W/v2a.nnue v2b=$W/v2b.nnue v2c=$W/v2c.nnue
} > $W/losses.txt 2>&1
for n in $NETS; do
  echo "== $n check on 100k validation positions" >> $W/export_checks.txt
  (cd $R && $PY tools/nnue/check_export.py --engine build/regicide --data $W/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4) >> $W/export_checks.txt
done
cat $W/losses.txt >> $W/status
BEST=$(grep -E '^v2[abc] ' $W/losses.txt | sort -k2 -n | head -n 2 | awk '{print $1}' | tr '\n' ' ')
st "selected for matches: $BEST"
[ -e $W/engines/egdb ] || ln -s ~/regicide-data/egdb $W/engines/egdb
cp $R/build/libregicide_cb.so $W/engines/rv2.so
mkdir -p $J/bin; cp $R/build/tools/match $J/bin/match-v2; cp $R/build/tools/cbhost $J/bin/cbhost-v2
until grep -q 'BASELINE end eb1037c-n4c kingsrow-2s' $J/status; do sleep 60; done
while [ -e ~/regicide-jobs/r3/matchlock ]; do sleep 60; done
touch $J/matchlock    # pausekeeper.sh stops datagen (and its bash parent) while this exists
sleep 8
st "matchlock taken; datagen state: $(for p in $(pgrep -f '[d]atagen-'); do echo -n "$p:$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
{
echo "bench nodes and nps, t3/nnue-v2 build, one binary, datagen paused, no matches (3 rounds)"
for i in 1 2 3; do
  echo "n4c (built-in)  $(printf 'bench\nquit\n' | $R/build/regicide 2>&1 | grep '^bench')"
  for n in $NETS; do echo "$n             $(printf "set evalfile $W/$n.nnue\nbench\nquit\n" | $R/build/regicide 2>&1 | grep '^bench')"; done
done
} > $W/bench.txt 2>&1
st "bench done"
for n in $BEST; do
  OUT=$W/matches/$n-vs-n4c
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs n4c (one rv2.so: A set evalfile $n.nnue, B built-in n4c)"
  $J/bin/match-v2 --cbhost $J/bin/cbhost-v2 -a $W/engines/rv2.so -b $W/engines/rv2.so --name-a $n --name-b n4c \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
rm -f $J/matchlock
st "R5_DONE"
