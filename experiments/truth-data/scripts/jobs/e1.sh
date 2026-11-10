#!/usr/bin/env bash
# E1 net n8a: n6c's data (all 10k files + gen5a x3) without records of <= 8 pieces, lambda 0.5.
D=~/regicide-data/train
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $D/${f}_n10k.bin"; done
G5="--data $D/gen5a_n50k.bin"
bash /mnt/c/regicide-match/jobs/truth/train.sh n8a $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9
