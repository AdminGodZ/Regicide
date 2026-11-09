#!/usr/bin/env bash
# Run inside an attached wsl.exe. 22:00 deadline plan: when d2_chain.sh has finished its 3 targeted pairs, stop it (its
# full Cake/KingsRow runs are skipped), then add targeted pairs (deep2, deep; 100 games each on the 5 ballots, same
# settings) as long as a pair can still finish by 21:15.
J=~/regicide-jobs/r4; J2=~/regicide-jobs/deep2; R=/mnt/c/regicide-match/jobs/deep2/run_ba3.sh
st() { echo "D2 $* $(date -Is)" >> $J/status; }
until grep -q '^D2 TARGETED_DONE' $J/status; do sleep 2; done
for p in $(pgrep -f 'bash /mnt/c/regicide-match/jobs/deep2/d2_chain.sh'); do kill $p; done
sleep 3
for p in $(pgrep -f 'match-747ab51 .*results/ba-deep2-full'); do kill $p; done
sleep 5
for d in ~/regicide-match/results/ba-deep2-full-cake; do [ -d $d ] && mv $d $J2/skipped-$(basename $d); done
st "d2_chain stopped after the targeted pairs (full Cake/KingsRow runs skipped for the 22:00 deadline)"
export OPENINGS=$J2/target_openings.txt
DEADLINE=$(date -d 'today 21:15' +%s)
i=4; PAIR=600   # first estimate of one pair's duration, s
while [ $(( $(date +%s) + PAIR )) -le $DEADLINE ]; do
  t0=$(date +%s)
  bash $R 6bb8c6c-deep2-egdb8 d2t$i cake 0.5
  bash $R 6bb8c6c-egdb8 d1t$i cake 0.5
  PAIR=$(( $(date +%s) - t0 + 30 )); i=$((i + 1))
done
st "TARGETED_EXT_DONE: $((i - 1)) pairs"
