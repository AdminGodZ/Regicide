#!/bin/bash
# Real-database checks for experiments/wld-conversion/RESULTS.md (2..7 pieces WLD, MTC up to 6).
#   run_real.sh <conversion_test binary> <label> <database dir>
set -e
bin=$1; label=$2; dir=$3
out=$(dirname "$0")/logs/$label
mkdir -p "$out"
for nodes in 5000 20000 100000; do
  # R6: 6-piece wins with the engine's MTC capped at 4 (two pieces short, as 8 pieces against MTC 6); the
  # opponent plays MTC-optimally.
  $bin --dir "$dir" --pieces 6 --sample 400 --cap 4 --nodes $nodes --role both --seed 1 \
    > "$out/R6_cap4_${nodes}.txt" 2>&1 || true
  # R7: 7-piece wins with the real MTC (6 pieces); no MTC exists for 7 pieces, so the opponent searches
  # 100k nodes per move there (with this binary's search) and plays MTC-optimally from 6 pieces down.
  $bin --dir "$dir" --pieces 7 --sample 400 --nodes $nodes --oracle-nodes 100000 --role both --seed 1 \
    > "$out/R7_${nodes}.txt" 2>&1 || true
done
