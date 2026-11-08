# After nn1 training: export + exact check, loss comparison on the NNUE validation set, bench (gen3a paused).
N=~/regicide-nnue-work
R=~/regicide-nnue
cd $R
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
grep -E '^(train|epoch|wrote)' $N/runs/nn1.log | tail -n 3
echo "== export + check_export (gen2b file, 1000 positions; then 100k positions of the validation set)"
.venv/bin/python tools/nnue/check_export.py --engine build/regicide --data ~/regicide-data/train/gen2b_n10k.bin \
  --checkpoint $N/runs/nn1/last.pt --net $N/nn1.nnue 2>&1 | tail -n 6
cmp $N/nn1.nnue $N/runs/nn1/net_e040.nnue && echo "nn1.nnue identical to net_e040.nnue"
ls -la $N/nn1.nnue; sha256sum $N/nn1.nnue | cut -c1-16
echo "== losses on the NNUE validation set"
.venv/bin/python ~/regicide-jobs/a44591e/compare_loss.py $N/runs/nn1/config.json $N build/regicide \
  nn1=$N/nn1.nnue builtin_bd2ef1d_G2c=builtin a44591e_w_lr0.03_wd0=$HOME/regicide-gen2/experiments/gen1full/wd/w_lr0.03_wd0.bin \
  G2d=$HOME/regicide-gen2-work/train/w_G2d.bin
.venv/bin/python tools/nnue/check_export.py --engine build/regicide --data $N/nnue_valid_1M.bin --checkpoint $N/runs/nn1/last.pt --n 100000 2>&1 | tail -n 4
echo "== bench (gen3a datagen paused)"
DG=$(pgrep -f '[d]atagen-bd2ef1d')
[ -n "$DG" ] && kill -STOP $DG && echo "paused datagen $DG $(date -Is)" >> $N/status
sleep 2
for i in 1 2 3; do
  echo "builtin: $(printf 'bench\nquit\n' | build/regicide | grep '^bench')"
  echo "nn1:     $(printf "set evalfile $N/nn1.nnue\nbench\nquit\n" | build/regicide | grep '^bench')"
done
for d in 15; do
  echo "builtin d$d: $(printf "bench $d\nquit\n" | build/regicide | grep '^bench')"
  echo "nn1 d$d:     $(printf "set evalfile $N/nn1.nnue\nbench $d\nquit\n" | build/regicide | grep '^bench')"
done
[ -n "$DG" ] && kill -CONT $DG && echo "resumed datagen $DG $(date -Is)" >> $N/status
