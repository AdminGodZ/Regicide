#!/usr/bin/env bash
L=~/regicide-jobs/booklab
nice python3 /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/dbscan.py --results ~/regicide-match/results \
  --scan $L/bin/blk-dbscan --egdb ~/regicide-data/egdb --out $L/wp8 --jobs 4 > $L/wp8.log 2>&1
echo "WP8 dbscan done rc=$? $(date -Is)" >> $L/status
