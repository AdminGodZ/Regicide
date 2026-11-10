#!/usr/bin/env bash
# lead's question on database-lost positions in step 4: quiet, no-dedup scan of the s4 runs + per-game analysis
set -e
W=~/regicide-booklab; L=~/regicide-jobs/booklab; O=$L/s4_quiet
cp /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/dbscan.cpp $W/tools/booklab/
bash /mnt/c/regicide-match/jobs/booklab/install_src.sh > /dev/null
bash /mnt/c/regicide-match/jobs/booklab/build_tree.sh 2 > /dev/null
cp $W/build-linux/booklab_dbscan $L/bin/blk-dbscan
rm -rf $O; mkdir -p $O/runs
for d in ~/regicide-match/results/bl-s4-*; do ln -s $d $O/runs/$(basename $d); done
python3 $W/tools/booklab/dbscan.py --results $O/runs --scan $L/bin/blk-dbscan --egdb ~/regicide-data/egdb --out $O \
  --jobs 4 --exclude "" --no-dedup --quiet > $O/dbscan.log 2>&1
tail -2 $O/dbscan.log
python3 $W/tools/booklab/entered_losses.py --scan-tsv $O/dbscan_games.tsv --results ~/regicide-match/results \
  --replay $W/build-linux/booklab_replay --out $O/entered_losses.md --list $O/entered_losses.txt
