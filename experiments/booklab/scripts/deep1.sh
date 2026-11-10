#!/usr/bin/env bash
# deep1.sh NAME FEN NODES: all-moves analysis of one position with egdb + 2 GB hash (blk-bookgen, registered pattern)
L=~/regicide-jobs/booklab; mkdir -p $L/deep
echo "$2" > $L/deep/$1.roots
nice $L/bin/blk-bookgen --roots $L/deep/$1.roots --nodes $3 --expansions 0 --threads 6 --hash 2048 \
  --egdb ~/regicide-data/egdb --egdb-cache 2048 --out $L/deep/$1.bin > $L/deep/$1.log 2>&1
echo "$2" | ~/regicide-booklab/build-linux/booklab_show $L/deep/$1.bin > $L/deep/$1.txt
echo "DEEP $1 done $(date -Is)" >> $L/status
