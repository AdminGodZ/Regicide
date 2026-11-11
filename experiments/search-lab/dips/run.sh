#!/bin/bash
# dips test: one config on one core at 1M, 3M, 10M nodes (or the given list).
D=~/regicide-jobs/dips; B=~/regicide-jobs/searchlab/bin/posbench-dips; core=$1; name=$2; params=$3; shift 3
for n in "$@"; do
  taskset -c $core $B --tsv $D/dips.tsv --nodes $n --hash 256 --params "$params" --egdb ~/regicide-data/egdb -v > $D/out/${name}_n$n.txt 2>&1
done
echo done > $D/out/$name.done
