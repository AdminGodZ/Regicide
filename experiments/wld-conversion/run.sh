#!/bin/bash
# Measurements A-D for experiments/wld-conversion/RESULTS.md.
#   run.sh <conversion_test binary> <label> <dir with generated 2..5-piece tables, built if missing>
# The tables come from `conversion_test --dir D --build 5` (WLD and MTC up to 5 pieces, ~4 s, ~60 MB).
set -e
bin=$1; label=$2; dir=$3
out=$(dirname "$0")/logs/$label
mkdir -p "$out"
$bin --dir "$dir" --build 5 --pieces 2 --sample 4 --nodes 1000 > /dev/null 2>&1 || true
for nodes in 5000 20000 100000; do
  # A: the engine converts 3- and 4-piece wins without MTC (cap 0) and with 2-piece MTC (cap 2).
  for cap in 0 2; do
    $bin --dir "$dir" --pieces 3,4 --sample 400 --cap $cap --nodes $nodes --role win --seed 1 \
      > "$out/A_cap${cap}_${nodes}.txt" 2>&1 || true
  done
  # B: the engine defends 3- and 4-piece losses (cap 0) against the MTC-optimal attacker.
  $bin --dir "$dir" --pieces 3,4 --sample 400 --cap 0 --nodes $nodes --role lose --seed 1 \
    > "$out/B_cap0_${nodes}.txt" 2>&1 || true
  # C: 5-piece wins with MTC up to 4 pieces: the engine has to steer into the MTC region.
  $bin --dir "$dir" --pieces 5 --sample 400 --cap 4 --nodes $nodes --role both --seed 1 \
    > "$out/C_cap4_${nodes}.txt" 2>&1 || true
done
