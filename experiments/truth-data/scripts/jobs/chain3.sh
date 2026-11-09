#!/usr/bin/env bash
# E3 net n8c: n8b's data (relabelled, >= 9 pieces) with gen5a (x3) and gen4a rescored (rescore_chain.sh), lambda 0.5,
# proven records trained on their exact result. Then its match vs n6c.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
until [ -s $O/gen5a_n50k_e3.bin ] && [ -s $O/gen4a_n10k_e3.bin ]; do sleep 60; done
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
ALL="$ALL --data $O/gen4a_n10k_e3.bin"
G5="--data $O/gen5a_n50k_e3.bin"
bash /mnt/c/regicide-match/jobs/truth/train.sh n8c $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 ${N8C_EXTRA:-}
bash /mnt/c/regicide-match/jobs/truth/match.sh n8c 0.1
st "CHAIN3_DONE"
