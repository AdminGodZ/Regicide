#!/usr/bin/env bash
# wait.sh PATTERN MINUTES: wait until PATTERN appears in ~/regicide-jobs/booklab/status (or timeout)
end=$(( $(date +%s) + ${2:-60} * 60 ))
until grep -q "$1" ~/regicide-jobs/booklab/status; do
  [ $(date +%s) -gt $end ] && { echo "TIMEOUT waiting for $1"; exit 1; }
  sleep 20
done
grep "$1" ~/regicide-jobs/booklab/status | tail -1 | cut -c1-250
