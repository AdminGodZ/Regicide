#!/usr/bin/env bash
# wait.sh PATTERN MINUTES: wait until PATTERN appears in ~/regicide-jobs/ponder/status
end=$(( $(date +%s) + ${2:-60} * 60 ))
until grep -q "$1" ~/regicide-jobs/ponder/status; do
  [ $(date +%s) -gt $end ] && { echo "TIMEOUT waiting for $1"; tail -3 ~/regicide-jobs/ponder/status; exit 1; }
  sleep 20
done
grep "$1" ~/regicide-jobs/ponder/status | tail -1 | cut -c1-400
