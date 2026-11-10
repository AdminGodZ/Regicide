#!/usr/bin/env bash
# tick.sh MINUTES: read-only wait: return early when the chain is done or a new PT end line appears; print status
S=~/regicide-jobs/ponder/status
n0=$(grep -c "PT end\|PT chain" $S); end=$(( $(date +%s) + $1 * 60 ))
while [ $(date +%s) -lt $end ]; do
  [ $(grep -c "PT end\|PT chain" $S) -gt $n0 ] && break
  sleep 20
done
grep "PT end\|PT start\|PT chain\|wait-lock" $S | tail -3 | cut -c1-330
for r in $(ls -td ~/regicide-match/results/pt-s2-* 2>/dev/null | head -1); do echo "$(basename $r): $(ls $r/games 2>/dev/null | wc -l) game files"; done
