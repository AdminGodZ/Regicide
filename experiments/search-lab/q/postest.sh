#!/bin/bash
# Q position test: each part on its own E-core; configs in order (cheap first).
Q=~/regicide-jobs/q; B=~/regicide-jobs/searchlab/bin/posbench-q
k=$1
for cfg in "0 1000000 64" "1 1000000 64" "0 20000000 128" "1 20000000 128" "0 100000000 256" "1 100000000 256"; do
  set -- $cfg
  taskset -c $((12+k)) $B --fens $Q/part_$k --nodes $2 --hash $3 --params "prune_forcing=$1" --egdb ~/regicide-data/egdb -v > $Q/out/pf$1_n$2_part$k.txt 2>&1
done
echo done > $Q/out/part$k.done
