#!/usr/bin/env bash
# Audit: E0 pilot again with the label.hpp rule (truth pilot of the merged branch), same 200k records (seed 11, same
# files), 10k/50k/200k nodes, 6 threads taken from E5's gen6b (stopped meanwhile, restarted with seed 613 afterwards).
source /mnt/c/regicide-match/jobs/truth/common.sh
D=~/regicide-data/train; cp ~/regicide-truth/build/truth $J/bin/truth-tr3
for p in $(pgrep -f "e5_gen.sh gen6b"); do kill $p; done; pkill -CONT -x datagen-tr5; pkill -x datagen-tr5; sleep 2
st "pilot2 start (gen6b stopped)"
F=""; for f in gen1_n10k gen1b_n10k gen1c_n10k gen2a_n10k gen2b_n10k gen3a_n10k gen3b_n10k gen3c_n10k gen3d_n10k gen4a_n10k gen5a_n50k gen5c_n50k; do F="$F $D/$f.bin"; done
$J/bin/truth-tr3 pilot --egdb ~/regicide-data/egdb --n 200000 --min 9 --max 14 --nodes 10000,50000,200000 --threads 6 --seed 11 --out $J/pilot2.csv $F > $J/pilot2.out 2>&1
st "pilot2 end rc=$?"
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 613 6
