#!/usr/bin/env bash
# P3: sweeps on the E2 data (relabelled results, >= 9 pieces), n6c's recipe otherwise:
#   n8bk130: K 130 (lambda 0.5);  n8bl6: lambda 0.6 (K 102.4);  n8bl7: lambda 0.7 (K 102.4). Each matched vs n6c (0.1 s).
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
T=/mnt/c/regicide-match/jobs/truth
bash $T/train.sh n8bk130 $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k 130
bash $T/match.sh n8bk130 0.1 &
bash $T/train.sh n8bl6 $ALL $G5 $G5 $G5 --lambda 0.6 --min-pieces 9 --k $K
wait; bash $T/match.sh n8bl6 0.1 &
bash $T/train.sh n8bl7 $ALL $G5 $G5 $G5 --lambda 0.7 --min-pieces 9 --k $K
wait; bash $T/match.sh n8bl7 0.1
st "P3_E2_DONE"
