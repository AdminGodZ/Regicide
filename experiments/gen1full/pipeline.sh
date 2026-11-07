#!/usr/bin/env bash
# gen1full pipeline (runs detached): train B70 on all gen1 data -> build B70 .so -> match B70 vs B13 -> start gen2a datagen.
# Steps whose output already exists are skipped (resumable).
J=~/regicide-jobs/gen1full
D=~/regicide-data/train
W=~/regicide-gen1full-work
SRC=~/regicide-gen1full
WT=~/regicide-gen1full-B70
SHA=2732f0d
st() { echo "$* $(date -Is)" >> $J/status; }
cd $W || exit 1

# wait for gen1c datagen
until grep -q '^done:' ~/regicide-jobs/gen1/datagen_c.log && ! pgrep -f '[d]atagen-b71dfa9' > /dev/null; do sleep 30; done

if ! grep -q '^wrote' $W/train_B70.log 2>/dev/null; then
  st "start train B70"
  $J/bin/train-$SHA --data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --out $W/w_B70.bin \
    --epochs 8 --lr 0.01 --batch 65536 --lambda 0.7 --threads 12 > $W/train_B70.log 2>&1
  st "end train B70 rc=$?"
fi
grep -q '^wrote' $W/train_B70.log || { st "train B70 failed"; exit 1; }

# B13 (the built-in w_B.bin) scored on the same split: --init with 0 epochs reports its epoch-0 valid loss.
if ! grep -q 'epoch 0' $W/eval_B13_fullsplit.log 2>/dev/null; then
  st "start eval B13 on full split"
  $J/bin/train-$SHA --data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --out $W/scratch_B13_e0.bin \
    --init $SRC/experiments/eval-gen1/w_B.bin --epochs 0 --lr 0.01 --batch 65536 --lambda 0.7 --threads 12 > $W/eval_B13_fullsplit.log 2>&1
  st "end eval B13 rc=$?"
fi

if [ ! -e $W/engines/B70.so ]; then
  st "build B70"
  cp $W/w_B70.bin $WT/src/weights/eval.bin
  cmake -S $WT -B $WT/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $W/build_B70.log 2>&1 &&
    ninja -C $WT/build -j 4 regicide_cb datagen regicide >> $W/build_B70.log 2>&1 || { st "build B70 failed"; exit 1; }
  cp $WT/build/libregicide_cb.so $W/engines/B70.so
  st "built B70 sha256 $(sha256sum $WT/src/weights/eval.bin | cut -c1-16)"
fi

OUT=$W/matches/B70-vs-B13
if ! grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null; then
  st "start match B70 vs B13"
  rm -rf $OUT
  $SRC/build/tools/match -a $W/engines/B70.so -b $W/engines/B13.so --name-a B70 --name-b B13 \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings $W/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $W/matches/B70-vs-B13.log 2>&1
  st "end match rc=$?"
fi
grep -q 'Games: 1610 of 1610' $OUT/summary.txt || { st "match incomplete"; exit 1; }

# gen2a datagen with the better weights (pentanomial Elo point estimate of B70 vs B13).
ELO=$(sed -n 's/^Elo (pentanomial, pairs): \([-+0-9.]*\).*/\1/p' $OUT/summary.txt)
if awk "BEGIN{exit !($ELO > 0)}"; then BEST=B70; WF=$W/w_B70.bin; else BEST=B13; WF=$SRC/experiments/eval-gen1/w_B.bin; fi
OUTD=$D/gen2a_n10k.bin
[ -e $OUTD ] && { st "gen2a exists, not starting"; exit 0; }
mkdir -p $J/gen2
cp $WF $J/gen2/w_gen2a_eval_$BEST.bin
st "gen2a datagen ($SHA) eval $BEST (B70 vs B13 Elo $ELO) seed 31 threads 12 games 300000 start"
$J/bin/datagen-$SHA --games 300000 --nodes 10000 --threads 12 --seed 31 --evalfile $J/gen2/w_gen2a_eval_$BEST.bin \
  --out $OUTD > $J/gen2/datagen_2a.log 2>&1
st "gen2a datagen end rc=$?"
