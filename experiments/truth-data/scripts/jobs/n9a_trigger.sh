#!/usr/bin/env bash
# n9a_trigger.sh: run n9a when n9h is done (status "n9h DONE") and E5 holds >= 30M
# game-deduplicated records (checked exactly once the raw count passes 33M, then every 20 minutes).
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
st "n9a trigger armed"
while :; do
  if grep -q "TR n9h DONE" $J/status; then
    raw=$(( ( $(stat -c %s $O/gen6a_n50k_egdb.bin) + $(stat -c %s $O/gen6b_n50k_egdb.bin) ) / 16 ))
    if [ $raw -ge 33000000 ]; then
      dd=$(nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/unique_count.py $O/gen6a_n50k_egdb.bin $O/gen6b_n50k_egdb.bin | grep -o "distinct: [0-9]*" | grep -o "[0-9]*")
      st "n9a trigger: $raw raw, $dd game-deduplicated"
      [ "${dd:-0}" -ge 30000000 ] && break
      sleep 1200; continue
    fi
  fi
  sleep 300
done
bash /mnt/c/regicide-match/jobs/truth/n9.sh n9a
