#!/usr/bin/env bash
# Control for E1: n8ak = n8a with K fixed to n6c's 102.4 (filtering refits K to 77.4, which changes the eval scale).
source /mnt/c/regicide-match/jobs/truth/common.sh
D=~/regicide-data/train
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $D/${f}_n10k.bin"; done
G5="--data $D/gen5a_n50k.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
bash /mnt/c/regicide-match/jobs/truth/train.sh n8ak $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K
bash /mnt/c/regicide-match/jobs/truth/match.sh n8ak 0.1
st "CHAIN4_DONE"
