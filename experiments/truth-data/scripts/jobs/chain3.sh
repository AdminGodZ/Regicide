#!/usr/bin/env bash
# E3 net n8c: n8b's data (relabelled, >= 9 pieces) with gen5a (x3) and gen4a rescored (rescore_chain.sh), lambda 0.5,
# proven records trained on their exact result; n8c with K refitted (as n8a/n8b), n8ck with n6c's K 102.4 (n8ak/n8bn).
# Then their matches vs n6c. The rescore runs with 6 threads while these train, 8 otherwise.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
until [ -s $O/gen5a_n50k_e3.bin ] && [ -s $O/gen4a_n10k_e3.bin ]; do sleep 60; done
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
ALL="$ALL --data $O/gen4a_n10k_e3.bin"
G5="--data $O/gen5a_n50k_e3.bin"
bash /mnt/c/regicide-match/jobs/truth/rs_threads.sh 6   # 6 + 2 training threads = my budget of 8
K=$(python3 -c "import json; print(json.load(open('$HOME/regicide-nnue6-work/runs/n6c/config.json'))['K'])")
bash /mnt/c/regicide-match/jobs/truth/train.sh n8c $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9
bash /mnt/c/regicide-match/jobs/truth/train.sh n8ck $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9 --k $K
bash /mnt/c/regicide-match/jobs/truth/rs_threads.sh 8
bash /mnt/c/regicide-match/jobs/truth/match.sh n8c 0.1
bash /mnt/c/regicide-match/jobs/truth/match.sh n8ck 0.1
st "CHAIN3_DONE"
