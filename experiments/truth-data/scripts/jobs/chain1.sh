#!/usr/bin/env bash
# After n8a's training: train n8b (E2), then match n8a and n8b vs n6c (0.1 s/move), no training during matches.
source /mnt/c/regicide-match/jobs/truth/common.sh
until grep -q '^TR train n8a exported' $J/status; do sleep 20; done
bash /mnt/c/regicide-match/jobs/truth/e2.sh
bash /mnt/c/regicide-match/jobs/truth/match.sh n8a 0.1
bash /mnt/c/regicide-match/jobs/truth/match.sh n8b 0.1
st "CHAIN1_DONE"
