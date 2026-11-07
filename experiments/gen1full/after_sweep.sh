#!/usr/bin/env bash
# After the wd sweep: one extra info point (lr 0.1, wd 0), then once bookgen has finished (file check / PID 52671 gone)
# match the sweep's best .so vs B13 with the B70-vs-B13 settings.
W=$HOME/regicide-gen1full-work
WD=$HOME/regicide-wd
D=$HOME/regicide-data/train
until grep -q SWEEP_DONE $W/wd/status; do sleep 30; done
$WD/build/train --data $D/gen1_n10k.bin --data $D/gen1b_n10k.bin --data $D/gen1c_n10k.bin --out $W/wd/w_lr0.1_wd0.bin \
  --epochs 8 --lr 0.1 --batch 65536 --lambda 0.7 --l2 0 --l2-table 0 --wd 0 --wd-table 0 --threads 6 > $W/wd/xtra_train_lr0.1_wd0.log 2>&1
echo "end xtra lr0.1_wd0 rc=$? $(date -Is)" >> $W/wd/status
SO=$(ls $W/engines/W_*.so 2>/dev/null | head -1)
[ -z "$SO" ] && { echo "no W .so; no match" >> $W/wd/status; exit 0; }
A=$(basename $SO .so)
until grep -q '^wrote ' ~/regicide-jobs/gen1full/bookgen.log || [ ! -d /proc/52671 ]; do sleep 30; done
OUT=$W/matches/$A-vs-B13
echo "start match $A vs B13 $(date -Is)" >> $W/wd/status
rm -rf $OUT
~/regicide-gen1full/build/tools/match -a $SO -b $W/engines/B13.so --name-a $A --name-b B13 \
  --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" \
  --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
  --openings $W/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $W/matches/$A-vs-B13.log 2>&1
echo "end match rc=$? $(date -Is)" >> $W/wd/status
echo "AFTER_DONE $(date -Is)" >> $W/wd/status
