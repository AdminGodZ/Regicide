#!/usr/bin/env bash
# P5: wide nets on the best v1 recipe and data. p5.sh BEST DATA
#   BEST: the best v1 net (n8bn or n8c); DATA: e2 (n8bn's files) or e3 (n8c's files).
# Nets: w2032 = FT 2048, L1 32; w1064 = FT 1024, L1 64; w2064 = FT 2048, L1 64 (else BEST's recipe: K 102.4, lambda 0.5,
# >= 9 pieces, 40 x 100M). Export checks against a REGICIDE_NNUE_MAX_FT=2048 build (build-ft2048).
# Speed: nodes/s of `bench` with each net (3 runs each, median), under the timing lock + matchlock.
# Matches vs BEST on one rft2048.so for both sides: 0.1 s (805 pairs) and 0.5 s (805 pairs in 4 chunks).
source /mnt/c/regicide-match/jobs/truth/common.sh
BEST=$1; DATA=$2
O=~/regicide-data/truth; W=~/regicide-truth-work; T=/mnt/c/regicide-match/jobs/truth; SO=$W/engines/rft2048.so
E=~/regicide-truth/build-ft2048/regicide
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
if [ $DATA = e3 ]; then ALL="$ALL --data $O/gen4a_n10k_e3.bin"; G5="--data $O/gen5a_n50k_e3.bin"
else ALL="$ALL --data $O/gen4a_n10k_e2.bin"; G5="--data $O/gen5a_n50k_e2.bin"; fi
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
st "P5 start: best v1 $BEST, data $DATA"
for spec in "w2032 2048 32" "w1064 1024 64" "w2064 2048 64"; do
  set -- $spec
  EVAL_ENGINE=$E bash $T/train.sh $1 $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K --ft $2 --l1 $3 || { st "P5: $1 failed its export check"; continue; }
done
# nodes/s
speed() {
  for n in $BEST w2032 w1064 w2064; do
    [ -s $W/$n.ok ] || continue
    for i in 1 2 3; do printf 'set book off\nset evalfile %s\nbench\nquit\n' $W/$n.nnue | $E 2>&1 | grep -o 'bench: .*' | sed "s/^/$n run $i: /"; done
  done > $W/logs/p5_speed.txt
}
with_timing speed
st "P5 speed: $(awk '{print $1, $NF, $(NF-1)}' $W/logs/p5_speed.txt | tr '\n' ';')"
for n in w2032 w1064 w2064; do
  [ -s $W/$n.ok ] || continue
  st "match $n-vs-$BEST queued"
  with_timing selfplay_match $n $W/$n.nnue $W/matches/$n-vs-$BEST $SO 0.1 $BEST $W/$BEST.nnue
  st "match $n-vs-$BEST end: $(grep '^Elo (pent' $W/matches/$n-vs-$BEST/summary.txt)"
done
C=$W/chunks; [ -s $C/op4_a ] || split -n l/4 -a 1 ~/regicide-gen2-work/openings4.txt $C/op4_
for n in w2032 w1064 w2064; do
  [ -s $W/$n.ok ] || continue
  for k in a b c d; do
    tag=$n-vs-$BEST-0.5-$k
    with_timing selfplay_match $n $W/$n.nnue $W/matches/$tag $SO 0.5 $BEST $W/$BEST.nnue $C/op4_$k
    st "match $tag end: $(grep '^Elo (pent' $W/matches/$tag/summary.txt)"
  done
  st "P5 $n vs $BEST 0.5 pooled: $(~/regicide-nnue/.venv/bin/python $T/pool_dirs.py $W/matches/$n-vs-$BEST-0.5-? | tail -n 1)"
done
st "P5_DONE"
