#!/usr/bin/env bash
# Audit requests (2026-11-10): n8bn2 = n8bn's exact recipe with training seed 2 (n8bn: seed 1), matched vs n6c at 0.1 s;
# and the direct E3-vs-E2 match n8c vs n8bn (805 pairs, 0.1 s). All on rtruth.so (fcf6acb), like n8bn's match.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; W=~/regicide-truth-work; T=/mnt/c/regicide-match/jobs/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
bash $T/train.sh n8bn2 $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K --seed 2 || exit 1
bash $T/match.sh n8bn2 0.1
st "match n8c-vs-n8bn queued"
with_timing selfplay_match n8c $W/n8c.nnue $W/matches/n8c-vs-n8bn $W/engines/rtruth.so 0.1 n8bn $W/n8bn.nnue
st "match n8c-vs-n8bn end: $(grep '^Elo (pent' $W/matches/n8c-vs-n8bn/summary.txt)"
st "AUDIT_DONE"
