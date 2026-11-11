#!/usr/bin/env bash
# n9a_variant.sh: after n9a, the lead's optional comparison (2026-11-11): n9a0 = n9a's recipe without the 6-16-ply
# random-opening data (E5 = only the records before gen6a 12,514,816 / gen6b 6,659,840, game-deduplicated the same way).
# Compared: the 9-12-piece draw proxy (pilot_draws.py) and the loss on the seed-reachable games of n9a's E5 validation
# tails (held out from n9a, never seen by n9a0). Also: start kinds of n9a's E5 training data.
source /mnt/c/regicide-match/jobs/truth/common.sh
set -o pipefail
O=~/regicide-data/truth; W=~/regicide-truth-work; T=/mnt/c/regicide-match/jobs/truth; PY=~/regicide-nnue/.venv/bin/python
S=$O/seeds_real_6_30.txt; TR=~/regicide-truth/build/truth
st "n9a_variant armed"
until grep -q "TR n9a DONE" $J/status; do sleep 60; done
SNAP=$O/e5_n9a0; mkdir -p $SNAP
head -c $(( 12514816 * 16 )) $O/gen6a_n50k_egdb.bin > $SNAP/gen6a_n50k_egdb.bin || exit 1
head -c $(( 6659840 * 16 )) $O/gen6b_n50k_egdb.bin > $SNAP/gen6b_n50k_egdb.bin || exit 1
$PY $T/tools/dedupe_games.py $SNAP $SNAP/gen6a_n50k_egdb.bin $SNAP/gen6b_n50k_egdb.bin > $J/n9a0_dedupe.txt || exit 1
rm -f $SNAP/gen6a_n50k_egdb.bin $SNAP/gen6b_n50k_egdb.bin
st "n9a0: E5 without the 6-16-ply part: $(tr '\n' ' ' < $J/n9a0_dedupe.txt)"
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
E5="--data $SNAP/gen6a_n50k_egdb_dedup.bin --data $SNAP/gen6b_n50k_egdb_dedup.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
touch $LOCKS/yield_truth
bash $T/train.sh n9a0 $ALL $G5 $G5 $G5 $E5 $E5 $E5 --lambda 0.5 --min-pieces 2 --k $K --groups 2,9 --group-weights 1,9 || { rm -f $LOCKS/yield_truth; st "n9a0: training FAILED"; exit 1; }
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
$PY $T/tools/pilot_draws.py n8q=$W/n8q.nnue n9h=$W/n9h.nnue n9a=$W/n9a.nnue n9a0=$W/n9a0.nnue > $J/pilot_draws_n9a0.md 2>&1
# seed-reachable games of n9a's E5 validation tails (offsets from unique_keys_n9a.txt)
A=$(grep "gen6a_n50k_egdb_dedup.bin: validation" $J/unique_keys_n9a.txt | grep -o "from record [0-9]*" | grep -o "[0-9]*")
B=$(grep "gen6b_n50k_egdb_dedup.bin: validation" $J/unique_keys_n9a.txt | grep -o "from record [0-9]*" | grep -o "[0-9]*")
$TR startkinds --seeds $S --threads 2 --from $A --out /tmp/n9a_sv_a.bin $O/e5_n9a/gen6a_n50k_egdb_dedup.bin > $J/startkinds_n9a.txt
$TR startkinds --seeds $S --threads 2 --from $B --out /tmp/n9a_sv_b.bin $O/e5_n9a/gen6b_n50k_egdb_dedup.bin >> $J/startkinds_n9a.txt
cat /tmp/n9a_sv_a.bin /tmp/n9a_sv_b.bin > $J/seeded_valid_n9a.bin; rm -f /tmp/n9a_sv_a.bin /tmp/n9a_sv_b.bin
$PY $T/tools/seeded_loss.py $J/seeded_valid_n9a.bin n8q=$W/n8q.nnue n9h=$W/n9h.nnue n9a=$W/n9a.nnue n9a0=$W/n9a0.nnue > $J/seeded_loss_n9a.md 2>&1
# start kinds of n9a0's E5 data too (training-set shares)
$TR startkinds --seeds $S --threads 2 $SNAP/gen6a_n50k_egdb_dedup.bin $SNAP/gen6b_n50k_egdb_dedup.bin > $J/startkinds_n9a0.txt
st "n9a_variant DONE"
