#!/usr/bin/env bash
# Extra work during/after the ae210fb baseline:
#  - gen3d datagen after gen3c (ae210fb built-in nn2b, seed 48, 300k games, 10k nodes, 12 threads)
#  - nn3e: FT 768 (825f993 scratch worktree), same data/settings as nn3a, CPU-capped (1 torch + 1 loader thread)
#  - after the baseline: export check, loss on the round-3 validation set, bench, match vs nn2b (datagen paused)
J=~/regicide-jobs/r3; W=~/regicide-nnue3-work; D=~/regicide-data/train
R=~/regicide-825; RE=~/regicide-825-ft768
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
st() { echo "$* $(date -Is)" | tee -a $J/status >> $W/status; }
st "extra start pid $$"
# gen3d chain (independent)
(
  until grep -q '^gen3c datagen end' $J/status; do sleep 60; done
  BIN=$J/bin/datagen-ae210fb
  [ -x $BIN ] || cp ~/regicide-match/builds/ae210fb/build-linux/datagen $BIN
  OUT=$D/gen3d_n10k.bin
  [ -e $OUT ] && { st "refusing: $OUT exists"; exit 1; }
  while [ -e $J/matchlock ]; do sleep 30; done
  st "gen3d datagen (ae210fb built-in NNUE nn2b) seed 48 threads 12 games 300000 start"
  $BIN --games 300000 --nodes 10000 --threads 12 --seed 48 --out $OUT > $J/datagen_gen3d.log 2>&1
  st "gen3d datagen end rc=$?"
) &
# FT 768 build
if [ ! -x $RE/build/regicide ]; then
  git -C ~/regicide-nnue worktree add -q --detach $RE 825f993
  cd $RE
  sed -i 's/^constexpr int FT = 512;/constexpr int FT = 768;/' src/nnue.hpp
  sed -i 's/^INPUTS, FT, L1, L2, BUCKETS = 128, 512, 32, 32, 8$/INPUTS, FT, L1, L2, BUCKETS = 128, 768, 32, 32, 8/' tools/nnue/nnue.py
  : > src/weights/net.bin
  {
    git diff --stat; grep -n '^constexpr int FT ' src/nnue.hpp; grep -n '^INPUTS, FT' tools/nnue/nnue.py
    cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
    nice cmake --build build -j 2 >> build.log 2>&1 || tail -n 30 build.log
    nice ctest --test-dir build -j 2 2>&1 | tail -n 3
    ./build/regicide bench | tail -n 1
    OMP_NUM_THREADS=1 $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-nnue-work/slice_gen2b_200k.bin 2>&1 | tail -n 4
  } > $W/build_ft768.txt 2>&1
  st "ft768 build: $(grep -c 'tests passed' $W/build_ft768.txt) ctest line, $(tail -n 1 $W/build_ft768.txt)"
fi
DATA=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b; do DATA="$DATA --data $D/${f}_n10k.bin"; done
if ! grep -q '^wrote ' $W/runs/nn3e.log 2>/dev/null; then
  st "start nn3e ($RE, FT 768, epochs 40, capped 1+1 threads)"
  (cd $RE && OMP_NUM_THREADS=1 $PY tools/nnue/train.py $DATA --out $W/runs/nn3e --epochs 40 --epoch-size 100000000 --batch 16384 \
     --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile --threads 1 --loader-threads 1 >> $W/runs/nn3e.log 2>&1)
  st "end nn3e rc=$?"
fi
export OMP_NUM_THREADS=1
{
  echo "== nn3e export check ($RE)"
  (cd $RE && $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-nnue-work/slice_gen2b_200k.bin \
     --checkpoint $W/runs/nn3e/last.pt --net $W/nn3e.nnue 2>&1 | tail -n 4)
  cmp $W/nn3e.nnue $(ls $W/runs/nn3e/net_e*.nnue | sort | tail -n 1) && echo "nn3e.nnue identical to the final net"
  echo "== nn3e check on 100k validation positions"
  (cd $RE && $PY tools/nnue/check_export.py --engine build/regicide --data $W/nnue_valid_1M.bin --checkpoint $W/runs/nn3e/last.pt --n 100000 2>&1 | tail -n 4)
} > $W/export_checks_nn3e.txt 2>&1
$PY $J/compare_loss.py $W/runs/nn3a/config.json $W $RE/build/regicide nn3e=$W/nn3e.nnue > $W/losses_nn3e.txt 2>&1
st "nn3e loss: $(grep '^nn3e ' $W/losses_nn3e.txt)"
cp $RE/build/libregicide_cb.so $W/engines/r825f993_ft768.so
until grep -q '^BASELINE_ALL_DONE' $J/status; do sleep 60; done
touch $J/matchlock
DG=$(pgrep -f '[d]atagen-(825f993|ae210fb)' | tr '\n' ' ')
[ -n "$DG" ] && kill -STOP $DG && st "paused datagen $DG for nn3e bench + match"
sleep 3
B=""
for i in 1 2 3; do
  a=$(printf 'bench\nquit\n' | $R/build/regicide | grep '^bench')
  e=$(printf "set evalfile $W/nn3e.nnue\nbench\nquit\n" | $RE/build/regicide | grep '^bench')
  c=$(printf "set evalfile $W/nn3c.nnue\nbench\nquit\n" | ~/regicide-825-ft1024/build/regicide | grep '^bench')
  B="$B$a|$e|$c\n"
done
printf "nn2b(builtin)|nn3e(FT768)|nn3c(FT1024)\n$B" > $W/bench_nn3e.txt
OUT=$W/matches/nn3e-vs-nn2b
if ! grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null; then
  rm -rf $OUT
  st "start match nn3e vs nn2b (A r825f993_ft768.so + set evalfile nn3e.nnue, B r825f993.so built-in nn2b)"
  $J/bin/match-825f993 -a $W/engines/r825f993_ft768.so -b $W/engines/r825f993.so --name-a nn3e --name-b nn2b \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/nn3e.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match nn3e rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
fi
DG2=$(pgrep -f '[d]atagen-(825f993|ae210fb)' | tr '\n' ' ')
[ -n "$DG2" ] && kill -CONT $DG2 && st "resumed datagen $DG2"
rm -f $J/matchlock
st "EXTRA_DONE"
wait
