#!/usr/bin/env bash
# Starts n9.sh n9h when E5 holds >= 15M game-deduplicated records (checked exactly once the raw count passes 17M), or at
# 08:00 on 2026-11-11 with what is there, whichever comes first.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
st "n9h trigger armed"
while :; do
  raw=$(( ( $(stat -c %s $O/gen6a_n50k_egdb.bin) + $(stat -c %s $O/gen6b_n50k_egdb.bin) ) / 16 ))
  if [ $(date +%s) -ge $(date -d "2026-11-11 08:00" +%s) ]; then st "n9h trigger: 08:00 reached at $raw raw records"; break; fi
  if [ $raw -ge 17000000 ]; then
    dd=$(nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/unique_count.py $O/gen6a_n50k_egdb.bin $O/gen6b_n50k_egdb.bin | grep -o "distinct: [0-9]*" | grep -o "[0-9]*")
    st "n9h trigger: $raw raw, $dd game-deduplicated"
    [ "${dd:-0}" -ge 15000000 ] && break
    sleep 600; continue
  fi
  sleep 120
done
bash /mnt/c/regicide-match/jobs/truth/n9.sh n9h
