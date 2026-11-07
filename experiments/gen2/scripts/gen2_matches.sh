#!/usr/bin/env bash
# Step 4: after the baseline (ALL_DONE baseline in match/status), match the 2 best gen2 candidates (by final valid loss,
# only those below the built-in's loss on the same split) vs the a44591e built-in eval.
# 805 4-ply openings x 2 colours, 0.1 s/move, -c 10, book off, threads 1, hash 128, 2-7 piece egdb symlink.
J=~/regicide-jobs/a44591e
SRC=~/regicide-gen2
T=~/regicide-gen2-work/train
W=~/regicide-gen2-work
WT=~/regicide-gen2-cand
st() { echo "$* $(date -Is)" >> $W/status; }
mkdir -p $W/engines $W/matches
[ -e $W/engines/egdb ] || ln -s ~/regicide-data/egdb $W/engines/egdb
cp -n ~/regicide-gen1full-work/openings4.txt $W/openings4.txt
cp $SRC/build/libregicide_cb.so $W/engines/a44591e.so
MATCH=$J/bin/match-a44591e-gen2
cp $SRC/build/tools/match $MATCH; cp $SRC/build/tools/cbhost $J/bin/cbhost
BASE=$(sed -n 's/^epoch 0  valid loss //p' $T/train_builtin_e0.log)
CANDS=$(for n in G2a G2b G2c G2d; do l=$(grep '^epoch' $T/train_$n.log | tail -1 | awk '{print $NF}'); \
  awk "BEGIN{exit !($l < $BASE)}" && echo "$l $n"; done | sort -n | head -2 | awk '{print $2}')
st "built-in valid $BASE; selected: $CANDS"
[ -d $WT ] || git -C $SRC worktree add --detach $WT a44591e
for n in $CANDS; do
  if [ ! -e $W/engines/$n.so ]; then
    cp $T/w_$n.bin $WT/src/weights/eval.bin
    cmake -S $WT -B $WT/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $W/build_$n.log 2>&1 &&
      ninja -C $WT/build -j 4 regicide_cb >> $W/build_$n.log 2>&1 || { st "build $n failed"; exit 1; }
    cp $WT/build/libregicide_cb.so $W/engines/$n.so
    git -C $WT checkout -- src/weights/eval.bin
    st "built $n.so (weights sha256 $(sha256sum $T/w_$n.bin | cut -c1-16))"
  fi
done
st "waiting for baseline ALL_DONE"
until grep -q '^ALL_DONE baseline' $J/match/status 2>/dev/null; do sleep 60; done
for n in $CANDS; do
  OUT=$W/matches/$n-vs-a44591e
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs a44591e (gen2b datagen running: $(pgrep -c -f '[d]atagen-a44591e'))"
  $MATCH -a $W/engines/$n.so -b $W/engines/a44591e.so --name-a $n --name-b a44591e \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings $W/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $W/matches/$n-vs-a44591e.log 2>&1
  st "end match $n rc=$? (gen2b datagen running: $(pgrep -c -f '[d]atagen-a44591e'))"
done
st "GEN2_MATCHES_DONE"
