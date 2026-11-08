#!/usr/bin/env bash
# NNUE round 3: waits for gen3b, starts gen3c datagen, trains nn3a (FT512 40ep), nn3c (FT1024 40ep),
# nn3d (FT512 L1 64 40ep), nn3b (FT512 80ep) on gen1..gen3b; export checks, losses on one validation set,
# bench, then matches of the selected nets vs nn2b (825f993 built-in) with gen3c paused.
J=~/regicide-jobs/r3
W=~/regicide-nnue3-work
D=~/regicide-data/train
R=~/regicide-825; RF=~/regicide-825-ft1024; RL=~/regicide-825-l1_64
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $W/runs $W/engines $W/matches
st() { echo "$* $(date -Is)" | tee -a $J/status >> $W/status; }
src_of() { case $1 in nn3c) echo $RF;; nn3d) echo $RL;; *) echo $R;; esac; }
st "r3_chain start pid $$"
until grep -q '^R3_BUILD_DONE' $J/status; do sleep 30; done
until grep -q '^gen3b datagen end' ~/regicide-nnue-work/status; do sleep 60; done
S=$(stat -c %s $D/gen3b_n10k.bin)
st "gen3b done: $S bytes, $((S/16)) records, rem $((S%16)); $(tail -n 1 ~/regicide-nnue-work/datagen_gen3b.log)"
pgrep -f '[d]atagen-825f993' > /dev/null || setsid nohup bash $J/gen3c.sh > /dev/null 2>&1 < /dev/null &
DATA=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b; do DATA="$DATA --data $D/${f}_n10k.bin"; done
run() {  # run NAME SRCDIR EPOCHS
  grep -q '^wrote ' $W/runs/$1.log 2>/dev/null && return 0
  st "start $1 ($2, epochs $3)"
  (cd $2 && $PY tools/nnue/train.py $DATA --out $W/runs/$1 --epochs $3 --epoch-size 100000000 --batch 16384 \
     --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile >> $W/runs/$1.log 2>&1)
  st "end $1 rc=$?"
}
run nn3a $R 40
run nn3c $RF 40
run nn3d $RL 40
run nn3b $R 80
st "NN3_TRAIN_DONE"
NETS="nn3a nn3b nn3c nn3d"
export OMP_NUM_THREADS=4
{
for n in $NETS; do
  src=$(src_of $n); last=$(ls $W/runs/$n/net_e*.nnue | sort | tail -n 1)
  echo "== $n export check ($src, final $(basename $last))"
  (cd $src && $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-nnue-work/slice_gen2b_200k.bin \
     --checkpoint $W/runs/$n/last.pt --net $W/$n.nnue 2>&1 | tail -n 4)
  cmp $W/$n.nnue $last && echo "$n.nnue identical to $(basename $last)"
done
} > $W/export_checks.txt 2>&1
cp ~/regicide-nnue2-work/nn2b.nnue $W/nn2b.nnue
{
echo "== losses on nn3a's validation set (lambda 0.7, K from nn3a config)"
cmp $W/nn2b.nnue $R/src/weights/net.bin && echo "(nn2b.nnue identical to the 825f993 built-in net)"
$PY $J/compare_loss.py $W/runs/nn3a/config.json $W $R/build/regicide nn2b=builtin nn3a=$W/nn3a.nnue nn3b=$W/nn3b.nnue
$PY $J/compare_loss.py $W/runs/nn3a/config.json $W $RF/build/regicide nn3c=$W/nn3c.nnue
$PY $J/compare_loss.py $W/runs/nn3a/config.json $W $RL/build/regicide nn3d=$W/nn3d.nnue
} > $W/losses.txt 2>&1
for n in $NETS; do
  src=$(src_of $n); echo "== $n check on 100k validation positions" >> $W/export_checks.txt
  (cd $src && $PY tools/nnue/check_export.py --engine build/regicide --data $W/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --n 100000 2>&1 | tail -n 4) >> $W/export_checks.txt
done
cat $W/losses.txt >> $W/status
L() { grep -E "^$1 " $W/losses.txt | awk '{print $2}'; }
BEST=$(grep -E '^nn3[abcd] ' $W/losses.txt | sort -k2 -n | head -n 2 | awk '{print $1}' | tr '\n' ' ')
# nn3c also when its loss beats the best FT-512 / L1-32 net (nn3a, nn3b)
B512=$(printf '%s\n%s\n' "$(L nn3a)" "$(L nn3b)" | sort -n | head -n 1)
case " $BEST " in *" nn3c "*) ;; *) awk "BEGIN{exit !($(L nn3c) < $B512)}" && BEST="$BEST nn3c";; esac
st "selected for matches: $BEST (best FT512 loss $B512)"
[ -e $W/engines/egdb ] || ln -s ~/regicide-data/egdb $W/engines/egdb
cp $R/build/libregicide_cb.so $W/engines/r825f993.so
cp $R/build/libregicide_cb.so $W/engines/r825f993_A.so
cp $RF/build/libregicide_cb.so $W/engines/r825f993_ft1024.so
cp $RL/build/libregicide_cb.so $W/engines/r825f993_l1_64.so
mkdir -p $J/bin; cp $R/build/tools/match $J/bin/match-825f993; cp $R/build/tools/cbhost $J/bin/cbhost
touch $J/matchlock
DG=$(pgrep -f '[d]atagen-825f993' | tr '\n' ' ')
[ -n "$DG" ] && kill -STOP $DG && st "paused datagen $DG for bench + matches"
sleep 3
B=""
for i in 1 2 3; do
  a=$(printf 'bench\nquit\n' | $R/build/regicide | grep '^bench')
  b=$(printf "set evalfile $W/nn3a.nnue\nbench\nquit\n" | $R/build/regicide | grep '^bench')
  b2=$(printf "set evalfile $W/nn3b.nnue\nbench\nquit\n" | $R/build/regicide | grep '^bench')
  c=$(printf "set evalfile $W/nn3c.nnue\nbench\nquit\n" | $RF/build/regicide | grep '^bench')
  d=$(printf "set evalfile $W/nn3d.nnue\nbench\nquit\n" | $RL/build/regicide | grep '^bench')
  B="$B$a|$b|$b2|$c|$d\n"
done
printf "nn2b(builtin)|nn3a|nn3b|nn3c(FT1024)|nn3d(L1 64)\n$B" > $W/bench.txt
st "bench done"
for n in $BEST; do
  case $n in nn3c) SO=$W/engines/r825f993_ft1024.so;; nn3d) SO=$W/engines/r825f993_l1_64.so;; *) SO=$W/engines/r825f993_A.so;; esac
  OUT=$W/matches/$n-vs-nn2b
  grep -q 'Games: 1610 of 1610' $OUT/summary.txt 2>/dev/null && continue
  rm -rf $OUT
  st "start match $n vs nn2b (A $(basename $SO) + set evalfile $n.nnue, B r825f993.so built-in nn2b)"
  $J/bin/match-825f993 -a $SO -b $W/engines/r825f993.so --name-a $n --name-b nn2b \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $W/$n.nnue" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime 0.1 -c 10 --pdn-comments -o $OUT > $OUT.log 2>&1
  st "end match $n rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
DG2=$(pgrep -f '[d]atagen-825f993' | tr '\n' ' ')
[ -n "$DG2" ] && kill -CONT $DG2 && st "resumed datagen $DG2"
rm -f $J/matchlock
st "NN3_MATCHES_DONE"
