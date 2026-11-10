#!/usr/bin/env bash
# Smoke test of datagen --egdb --seeds (1 thread, 50 games, 10k nodes); checks the records with truth/python.
source /mnt/c/regicide-match/jobs/truth/common.sh
cp ~/regicide-truth/build/datagen $J/bin/datagen-tr1
rm -f $J/dg_test.bin
st "datagen test start"
$J/bin/datagen-tr1 --games 50 --nodes 10000 --threads 1 --seed 5 --egdb ~/regicide-data/egdb --seeds ~/regicide-data/truth/seeds_real_6_40.txt --out $J/dg_test.bin > $J/dg_test.log 2>&1
~/regicide-nnue/.venv/bin/python - >> $J/dg_test.log <<PY
import sys; sys.path.insert(0, "$HOME/regicide-truth/tools/nnue"); import nnue, numpy as np
r = np.fromfile("$J/dg_test.bin", nnue.RECORD); p = nnue.piece_counts(r); pr = nnue.proven_of(r)
print("records", len(r), "min pieces", p.min(), "max pieces", p.max(), "proven", pr.sum(), "proven scores", sorted(set(r["score"][pr].tolist()))[:6], "stm values", sorted(set(r["stm"].tolist())))
print("pieces of proven", np.bincount(p[pr], minlength=25)[9:25].tolist())
PY
st "datagen test end: $(tail -n 2 $J/dg_test.log | head -n 1)"
