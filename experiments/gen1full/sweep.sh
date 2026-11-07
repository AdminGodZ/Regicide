#!/usr/bin/env bash
# Decoupled-wd sweep with the 903c510 trainer on gen1+gen1b+gen1c (l2 off). 4 threads per run, 3 runs at a time
# (the first two waves). Afterwards builds a .so for the best run, but only if its final valid loss < 0.5358.
WD=$HOME/regicide-wd
W=$HOME/regicide-gen1full-work/wd
D=$HOME/regicide-data/train
mkdir -p $W
echo "sweep start $(date -Is) commit $(git -C $WD rev-parse --short HEAD)" >> $W/status
run() {  # run <name> <lr> <wd>
  [ -f $W/w_$1.bin ] && grep -q '^wrote' $W/train_$1.log 2>/dev/null && return 0
  $WD/build/train --data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --out $W/w_$1.bin \
    --epochs 8 --lr $2 --batch 65536 --lambda 0.7 --l2 0 --l2-table 0 --wd $3 --wd-table $3 --threads 4 > $W/train_$1.log 2>&1
  echo "end $1 rc=$? $(date -Is)" >> $W/status
}
run lr0.01_wd0 0.01 0 & run lr0.01_wd1e-3 0.01 1e-3 & run lr0.003_wd0 0.003 0 & wait
run lr0.003_wd1e-3 0.003 1e-3 & run lr0.03_wd0 0.03 0 & wait
best=$(for f in $W/train_*.log; do n=$(basename $f .log); n=${n#train_}; l=$(grep '^epoch 8 ' $f | awk '{print $NF}'); [ -n "$l" ] && echo "$l $n"; done | sort -n | head -1)
echo "best $best" >> $W/status
set -- $best
if awk "BEGIN{exit !($1 < 0.5358)}"; then
  cp $W/w_$2.bin $WD/src/weights/eval.bin
  ninja -C $WD/build -j 4 regicide_cb > $W/build_so.log 2>&1 && cp $WD/build/libregicide_cb.so $HOME/regicide-gen1full-work/engines/W_$2.so &&
    echo "built W_$2.so $(date -Is)" >> $W/status
  git -C $WD checkout -- src/weights/eval.bin
else
  echo "no run beats 0.5358; no .so built" >> $W/status
fi
echo "SWEEP_DONE $(date -Is)" >> $W/status
