#!/usr/bin/env bash
# v2g: v2b's architecture (v2 2x1024 -> 16 -> 32, 8 buckets), 40 epochs, on all data incl. gen4a complete (data2),
# capped at 1 torch + 1 loader thread. Keeps the GPU busy after round 5b; then its loss on the v2d validation set.
J=~/regicide-jobs/r4; W=~/regicide-nnue5-work; R=~/regicide-v2; PY=~/regicide-nnue/.venv/bin/python
st() { echo "R5b $* $(date -Is)" | tee -a $J/status >> $W/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
st "start v2g (--arch v2 --ft 1024 --l1 16 --l2 32 --buckets 8, 40 epochs, data2)"
(cd $R && DATA_DIR=$W/data2 WORK=$W PY=$PY bash experiments/nnue_v2/scripts/train_v2.sh v2g --arch v2 --ft 1024 --l1 16 --l2 32 --buckets 8 > $W/v2g.train_v2.out 2>&1)
st "end v2g rc=$? $(grep -m1 'max |diff|' $W/runs/v2g.check.txt)"
export OMP_NUM_THREADS=4
$PY /mnt/c/regicide-match/jobs/r5/compare_loss.py $W/runs/v2d/config.json $W/val2 $R/build/regicide v2g=$W/v2g.nnue > $W/losses_v2g.txt 2>&1
(cd $R && $PY tools/nnue/check_export.py --engine build/regicide --data $W/val2/nnue_valid_1M.bin --checkpoint $W/runs/v2g/last.pt --n 100000 2>&1 | tail -n 4) > $W/export_checks_v2g.txt
st "v2g loss (v2d set): $(grep '^v2g ' $W/losses_v2g.txt)"
st "V2G_DONE"
