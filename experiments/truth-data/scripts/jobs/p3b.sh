#!/usr/bin/env bash
# P3 (lead's addition): n8p = n8bn's recipe (E2 data, >= 9 pieces, K 102.4, lambda 0.5) with phase-balanced batches:
# exactly 1/3 of every batch from each of 9-12, 13-16 and 17-24 pieces (nnue.Data groups). Matches vs n6c and vs
# n8bn (0.1 s, rtruth.so). If n8p wins (beats n8bn head to head by its point estimate, or scores more than n8bn's +4.5
# vs n6c), also n8p2 with the split 25% / 25% / 50%.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; W=~/regicide-truth-work; T=/mnt/c/regicide-match/jobs/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
h2h() {  # h2h A B: A vs B, both with set evalfile
  st "match $1-vs-$2 queued"
  with_timing selfplay_match $1 $W/$1.nnue $W/matches/$1-vs-$2 $W/engines/rtruth.so 0.1 $2 $W/$2.nnue
  st "match $1-vs-$2 end: $(grep '^Elo (pent' $W/matches/$1-vs-$2/summary.txt)"
}
elo() { grep -o '^Elo (pentanomial, pairs): [-+0-9.]*' $1 | awk '{print $4}'; }
bash $T/train.sh n8p $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K --groups 9,13,17 --group-weights 1,1,1
bash $T/match.sh n8p 0.1
h2h n8p n8bn
a=$(elo $W/matches/n8p-vs-n6c/summary.txt); b=$(elo $W/matches/n8p-vs-n8bn/summary.txt)
if python3 -c "import sys; sys.exit(0 if float('$b') > 0 or float('$a') > 4.5 else 1)"; then
  bash $T/train.sh n8p2 $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K --groups 9,13,17 --group-weights 1,1,2
  bash $T/match.sh n8p2 0.1
  h2h n8p2 n8bn
fi
st "P3B_DONE"
