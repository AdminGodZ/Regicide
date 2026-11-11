#!/usr/bin/env bash
# E2 net n8b: as n8a (no records of <= 8 pieces), with game results relabelled from the database (truth relabel).
O=~/regicide-data/truth
ALL=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do ALL="$ALL --data $O/${f}_n10k_e2.bin"; done
G5="--data $O/gen5a_n50k_e2.bin"
bash /mnt/c/regicide-match/jobs/truth/train.sh n8b $ALL $G5 $G5 $G5 --lambda 0.5 --min-pieces 9
