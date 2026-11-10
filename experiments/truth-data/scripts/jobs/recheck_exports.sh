#!/usr/bin/env bash
# Re-runs the (stricter) export check on nets already trained: python int vs engine AND simulated int (training) vs
# engine (bound 2 centi-men), plus "the matched file is the checkpoint's export". Writes NAME.ok only on a pass.
source /mnt/c/regicide-match/jobs/truth/common.sh
W=~/regicide-truth-work; T=~/regicide-truth; E=$T/build/regicide; PY=~/regicide-nnue/.venv/bin/python
out=$W/logs/export_rechecks.txt; : > $out
for n in "$@"; do
  chk=$(cd $T && OMP_NUM_THREADS=1 nice $PY tools/nnue/check_export.py --engine $E --data ~/regicide-nnue5-work/val2/nnue_valid_1M.bin --checkpoint $W/runs/$n/last.pt --net /tmp/recheck_$n.nnue --n 100000 2>&1); rc=$?
  same=no; cmp -s /tmp/recheck_$n.nnue $W/$n.nnue && same=yes
  { echo "== $n rc=$rc matched-file-is-checkpoint-export=$same"; echo "$chk" | tail -n 4; } >> $out
  if [ $rc = 0 ] && [ $same = yes ]; then echo "$chk" | tail -n 5 > $W/$n.ok; else rm -f $W/$n.ok; fi
done
st "export rechecks: $(grep -c 'rc=0 matched-file-is-checkpoint-export=yes' $out) of $# pass ($out)"
