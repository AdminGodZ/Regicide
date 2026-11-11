#!/usr/bin/env bash
# relabel_e2f.sh: rebuild the E2 files with the fixed truth relabel (own database value on every <= 8-piece record,
# tools/relabel.hpp) as ~/regicide-data/truth/NAME_e2f.bin; the old *_e2.bin stay (n8q and n9h were trained on them).
# Then: truth labelcheck on the new files (must show 0 wrong), and a record-by-record comparison with the old E2 file
# (only <= 8-piece records may differ). One thread at a time.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data; B=~/regicide-truth/build-m/truth; PY=~/regicide-nnue/.venv/bin/python
st "relabel_e2f start"
F=""
for f in gen1_n10k gen1b_n10k gen1c_n10k gen2a_n10k gen2b_n10k gen3a_n10k gen3b_n10k gen3c_n10k gen3d_n10k gen4a_n10k gen5a_n50k gen5c_n50k; do
  nice $B relabel --egdb $O/egdb --in $O/train/$f.bin --out $O/truth/${f}_e2f.bin.tmp >> $J/relabel_e2f.log 2>&1 || { st "relabel_e2f FAILED on $f"; exit 1; }
  mv $O/truth/${f}_e2f.bin.tmp $O/truth/${f}_e2f.bin
  nice $PY /mnt/c/regicide-match/jobs/truth/tools/e2_diff.py $O/truth/${f}_e2.bin $O/truth/${f}_e2f.bin >> $J/e2f_diff.txt 2>&1
  F="$F $O/truth/${f}_e2f.bin"
done
nice $B labelcheck --egdb $O/egdb $F > $J/labelcheck_e2f.md 2>&1
st "relabel_e2f end: $(tail -1 $J/labelcheck_e2f.md | cut -c1-120)"
