#!/usr/bin/env bash
# NNUE round 5b: v2d (v2 2x512 L1 32, 8 buckets, 80 epochs), v2e (v1 FT 1024, 60 epochs: n4c retrain on more data),
# v2f (v2 2x768 L1 16, 8 buckets, 40 epochs), all on gen1..gen4a (all complete).
# Lane 1 starts when round 5's training is done and gen4a is complete; lane 2 only after the round-5 matches
# (at most one trainer, about 1.2 cores, while matches run). Then losses of all round-5/5b nets + n4c on v2d's
# validation set, nps bench, and a match of the best 5b net vs n4c (or vs the round-5 winner, if one has CI low > 0).
J=~/regicide-jobs/r4; W=~/regicide-nnue5-work; D=~/regicide-data/train; R=~/regicide-v2
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/data2 $W/claims $W/val2
st() { echo "R5b $* $(date -Is)" | tee -a $J/status >> $W/status; }
st "r5b_chain start pid $$"
until grep -q '^gen4a datagen end' $J/status; do sleep 30; done
for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ln -sf $D/${f}_n10k.bin $W/data2/${f}_n10k.bin; done
S=$(stat -c %s $D/gen4a_n10k.bin); st "gen4a complete: $S bytes, $((S / 16)) records, rem $((S % 16))"
ls -lL $W/data2 > $W/data2_files.txt
args() { case $1 in v2d) echo "--arch v2 --ft 512 --l1 32 --l2 32 --buckets 8";; v2e) echo "--arch v1 --ft 1024 --l1 32 --l2 32 --buckets 8";; v2f) echo "--arch v2 --ft 768 --l1 16 --l2 32 --buckets 8";; esac; }
ep() { case $1 in v2d) echo 80;; v2e) echo 60;; *) echo 40;; esac; }
lane() {
  for n in v2d v2e v2f; do
    mkdir $W/claims/$n 2>/dev/null || continue
    st "lane $1 start $n ($(args $n), $(ep $n) epochs)"
    (cd $R && DATA_DIR=$W/data2 WORK=$W PY=$PY EPOCHS=$(ep $n) bash experiments/nnue_v2/scripts/train_v2.sh $n $(args $n) > $W/$n.train_v2.out 2>&1)
    st "lane $1 end $n rc=$? $(grep -m1 'max |diff|' $W/runs/$n.check.txt)"
  done
}
( until grep -q '^R5 R5_TRAIN_DONE' $J/status; do sleep 30; done; lane 1 ) &
L1=$!
( until grep -q '^R5 R5_DONE' $J/status; do sleep 30; done; lane 2 ) &
L2=$!
wait $L1 $L2
st "R5B_TRAIN_DONE"
export OMP_NUM_THREADS=4
{
for n in v2d v2e v2f; do
  last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1)
  echo "== $n ($(args $n), $(ep $n) epochs) export check on 100k of the last data file (train_v2.sh)"; cat $W/runs/$n.check.txt
  cmp $W/$n.nnue $last && echo "$n.nnue identical to $(basename $last)"
  echo "== $n check on the 200k gen2b slice"
  (cd $R && $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-nnue-work/slice_gen2b_200k.bin --checkpoint $W/runs/$n/last.pt 2>&1 | tail -n 4)
done
} > $W/export_checks_5b.txt 2>&1
{
echo "== losses on v2d's validation set (all data incl. full gen4a; lambda 0.7, K from v2d config), engine integer eval, one t3/nnue-v2 binary"
$PY /mnt/c/regicide-match/jobs/r5/compare_loss.py $W/runs/v2d/config.json $W/val2 $R/build/regicide n4c=builtin \
  v2a=$W/v2a.nnue v2b=$W/v2b.nnue v2c=$W/v2c.nnue v2d=$W/v2d.nnue v2e=$W/v2e.nnue v2f=$W/v2f.nnue
} > $W/losses_5b.txt 2>&1
for n in v2d v2e v2f; do
  echo "== $n check on 100k validation positions (v2d set)" >> $W/export_checks_5b.txt
  (cd $R && $PY tools/nnue/check_export.py --engine build/regicide --data $W/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4) >> $W/export_checks_5b.txt
done
cat $W/losses_5b.txt >> $W/status
BEST=$(grep -E '^v2[def] ' $W/losses_5b.txt | sort -k2 -n | head -n 1 | awk '{print $1}')
# opponent: the round-5 winner (CI lower bound > 0, best Elo) or n4c
OPP=n4c; OPPOPT=()
for m in $W/matches/v2[abc]-vs-n4c; do
  [ -f $m/summary.txt ] || continue
  read lo el <<< "$(grep '^Elo (pent' $m/summary.txt | sed -E 's/^Elo \(pentanomial, pairs\): ([-+0-9.]+) \[([-+0-9.]+), .*/\2 \1/')"
  n=$(basename $m | cut -d- -f1)
  if awk "BEGIN{exit !($lo > 0)}"; then
    if [ $OPP = n4c ] || awk "BEGIN{exit !($el > $OPPEL)}"; then OPP=$n; OPPEL=$el; fi
  fi
done
[ $OPP != n4c ] && OPPOPT=(--option-b "set evalfile $W/$OPP.nnue")
st "selected for match: $BEST vs $OPP"
while [ -e $J/matchlock ] || [ -e ~/regicide-jobs/r3/matchlock ]; do sleep 60; done
touch $J/matchlock
sleep 8
st "matchlock taken; datagen state: $(for p in $(pgrep -f '[d]atagen-'); do echo -n "$p:$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
{
echo "bench nodes and nps, t3/nnue-v2 build, one binary, no matches (3 rounds)"
for i in 1 2 3; do
  echo "n4c (built-in)  $(printf 'bench\nquit\n' | $R/build/regicide 2>&1 | grep '^bench')"
  for n in v2d v2e v2f; do echo "$n             $(printf "set evalfile $W/$n.nnue\nbench\nquit\n" | $R/build/regicide 2>&1 | grep '^bench')"; done
done
} > $W/bench_5b.txt 2>&1
st "bench done"
OUT=$W/matches/$BEST-vs-$OPP
if ! grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null; then
  rm -rf $OUT
  st "start match $BEST vs $OPP (one rv2.so: A set evalfile $BEST.nnue, B ${OPPOPT[*]:-built-in n4c})"
  $J/bin/match-v2 --cbhost $J/bin/cbhost-v2 -a $W/engines/rv2.so -b $W/engines/rv2.so --name-a $BEST --name-b $OPP \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$BEST.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" "${OPPOPT[@]}" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $BEST vs $OPP rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
fi
rm -f $J/matchlock
st "R5B_DONE"
