#!/usr/bin/env bash
# Control for E2: n8bk = n8b with K fixed to n8a's fitted 77.4 (the relabelled data refits K to 43.4, which changes
# the eval scale as well as the labels). Then its match vs n6c.
source /mnt/c/regicide-match/jobs/truth/common.sh
until grep -q '^TR train n8b exported' $J/status; do sleep 20; done
O=~/regicide-data/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-truth-work/runs/n8a/config.json'))['K'])")
bash /mnt/c/regicide-match/jobs/truth/train.sh n8bk $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K
bash /mnt/c/regicide-match/jobs/truth/match.sh n8bk 0.1
st "CHAIN2_DONE"
