#!/usr/bin/env bash
# n9.sh NAME: n9h / n9a = n8q's recipe (E2 files + gen5a_e2 x3, K 102.4, lambda 0.5, groups 2-8 / 9-24 with weights 1 : 9)
# plus the E5 data so far, game-deduplicated (scripts/dedupe_games.py over gen6a + gen6b), each file given 3 times.
# Then: export check (train.sh), sanity table, draw proxy at 9-12 pieces, ctest with the net built in, validation
# overlap of the E5 files; the net and logs are committed to t3/truth-data.
source /mnt/c/regicide-match/jobs/truth/common.sh
set -o pipefail
NAME=$1; O=~/regicide-data/truth; W=~/regicide-truth-work; T=/mnt/c/regicide-match/jobs/truth; PY=~/regicide-nnue/.venv/bin/python
SNAP=$O/e5_$NAME; mkdir -p $SNAP
st "$NAME: snapshot of E5"
for f in gen6a_n50k_egdb gen6b_n50k_egdb; do
  s=$(stat -c %s $O/$f.bin); s=$(( s / 16 * 16 ))
  head -c $s $O/$f.bin > $SNAP/$f.bin || exit 1
done
$PY $T/tools/dedupe_games.py $SNAP $SNAP/gen6a_n50k_egdb.bin $SNAP/gen6b_n50k_egdb.bin > $J/${NAME}_dedupe.txt || exit 1
rm -f $SNAP/gen6a_n50k_egdb.bin $SNAP/gen6b_n50k_egdb.bin
st "$NAME: E5 deduplicated: $(tr '\n' ' ' < $J/${NAME}_dedupe.txt)"
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
E5="--data $SNAP/gen6a_n50k_egdb_dedup.bin --data $SNAP/gen6b_n50k_egdb_dedup.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
touch $LOCKS/yield_truth   # training (2 threads) + losswalk (2): gen6b pauses, staying within 14 threads
bash $T/train.sh $NAME $ALL $G5 $G5 $G5 $E5 $E5 $E5 --lambda 0.5 --min-pieces 2 --k $K --groups 2,9 --group-weights 1,9 || { rm -f $LOCKS/yield_truth; st "$NAME: training or export check FAILED"; exit 1; }
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
bash $T/sanity.sh $J/sanity_$NAME.md n6c=$HOME/regicide-nnue6-work/n6c.nnue n8q=$W/n8q.nnue $NAME=$W/$NAME.nnue > /dev/null
$PY $T/tools/pilot_draws.py n6c=$HOME/regicide-nnue6-work/n6c.nnue n8q=$W/n8q.nnue $NAME=$W/$NAME.nnue > $J/pilot_draws_$NAME.md 2>&1
$PY $T/tools/unique_keys.py "E5_$NAME=$SNAP/gen6a_n50k_egdb_dedup.bin,$SNAP/gen6b_n50k_egdb_dedup.bin" > $J/unique_keys_$NAME.txt 2>&1
bash $T/embed_test.sh $NAME
st "$NAME DONE: $(grep '3 kings' $J/sanity_$NAME.md | head -n 1)"
