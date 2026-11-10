#!/usr/bin/env bash
# After gen4a's rescore (all that n8c needs): park the rescore chain (resumable) and give its 6 base-budget threads to
# E5 datagen (gen6a). Resume the rescore later with: bash /mnt/c/regicide-match/jobs/truth/rs_threads.sh 6
source /mnt/c/regicide-match/jobs/truth/common.sh
until grep -q "^TR rescore gen4a_n10k done" $J/status; do sleep 30; done
for p in $(pgrep -f "regicide-jobs/truth/rescore_chain.sh"); do kill $p; done
pkill -9 "^rescore-tr"; sleep 2   # SIGKILL: a SIGTERM can stay pending while the pausekeeper keeps it stopped
st "rescore chain parked after gen4a (resumable: rs_threads.sh N); its threads go to E5 gen6a"
bash /mnt/c/regicide-match/jobs/truth/e5_gen.sh gen6a_n50k_egdb datagen-tr6 611 6
