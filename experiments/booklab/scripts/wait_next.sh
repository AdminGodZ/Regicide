#!/usr/bin/env bash
# wait_next.sh MINUTES: wait until a new "BL end" line appears in the status log; print it
S=~/regicide-jobs/booklab/status
n0=$(grep -c "BL end" $S); end=$(( $(date +%s) + ${1:-9} * 60 ))
while [ $(grep -c "BL end" $S) -le $n0 ]; do
  [ $(date +%s) -gt $end ] && { echo "no new run end within $1 min ($(date +%H:%M))"; exit 0; }
  sleep 15
done
grep "BL end" $S | tail -n +$((n0+1)) | cut -c1-200
