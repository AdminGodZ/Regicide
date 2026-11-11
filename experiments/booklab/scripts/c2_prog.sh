#!/usr/bin/env bash
# progress of the WP12 c2 scoring: select.log tail, the running multiscore's elapsed and CPU time, its input size
L=~/regicide-jobs/booklab
tail -2 $L/wp12/c2/select.log
for p in $(pgrep -f "blk-multiscore-1c6e 20000000"); do
  echo "multiscore pid $p: $(ps -o etime=,time=,stat= -p $p)"
done
grep -c "^8f5621a2" $L/wp12/cache.tsv 2>/dev/null | sed 's/^/cached move scores of c2: /'
