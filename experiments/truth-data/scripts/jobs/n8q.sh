#!/usr/bin/env bash
# T4: n8q = n8bn's recipe and data (E2 relabelled, K 102.4, lambda 0.5) plus the relabelled records with <= 8 pieces as
# their own sampling group: groups 2-8 and 9-24 with weights SHARE : (1 - SHARE). The >= 9 records keep their natural
# proportions (one group), so within them the sampling equals n8bn's. NAME and SHARE weights as arguments.
#   n8q.sh n8q 1,9   (10%)      n8q.sh n8q5 1,19   (5%)
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; T=/mnt/c/regicide-match/jobs/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
bash $T/train.sh $1 $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 2 --k $K --groups 2,9 --group-weights $2 || exit 1
