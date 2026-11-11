#!/usr/bin/env bash
# E2 at n6c's K: n8bn = n8b's data (relabelled, >= 9 pieces) with K fixed to 102.4 (n8ak showed that the refit K of the
# filtered data, 77.4, costs about 4 Elo on the original labels). Then its match vs n6c.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
bash /mnt/c/regicide-match/jobs/truth/train.sh n8bn $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K
bash /mnt/c/regicide-match/jobs/truth/match.sh n8bn 0.1
st "CHAIN5_DONE"
