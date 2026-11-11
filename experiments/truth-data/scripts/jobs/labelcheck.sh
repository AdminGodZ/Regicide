#!/usr/bin/env bash
# labelcheck.sh: truth labelcheck over the E2 files of n8q's / n9h's recipe (and gen5c_e2, and the E5 files for the
# datagen path); one thread, base budget.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; B=~/regicide-truth/build-m/truth
st "labelcheck start"
F=""; for f in gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a; do F="$F $O/${f}_n10k_e2.bin"; done
F="$F $O/gen5a_n50k_e2.bin $O/gen5c_n50k_e2.bin"
nice $B labelcheck --egdb ~/regicide-data/egdb $F > $J/labelcheck_e2.md 2>&1
nice $B labelcheck --egdb ~/regicide-data/egdb $O/e5_n9h/gen6a_n50k_egdb_dedup.bin $O/e5_n9h/gen6b_n50k_egdb_dedup.bin > $J/labelcheck_e5.md 2>&1
st "labelcheck end rc=$?"
