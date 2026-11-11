#!/usr/bin/env bash
# wait_lines.sh FILE N MINUTES [STATUS_PATTERN]: wait until FILE has at least N lines, or STATUS_PATTERN appears in the
# booklab status, or MINUTES pass; prints the line count and any matching status lines.
F=$1; N=$2; M=$3; P=${4:-}
L=~/regicide-jobs/booklab
for i in $(seq $((M * 6))); do
  n=$(wc -l < "$F" 2>/dev/null || echo 0)
  [ "$n" -ge "$N" ] && break
  [ -n "$P" ] && grep -q "$P" $L/status && break
  sleep 10
done
echo "lines $(wc -l < "$F" 2>/dev/null || echo 0) of $N"
[ -n "$P" ] && grep "$P" $L/status | cut -c1-150
true
