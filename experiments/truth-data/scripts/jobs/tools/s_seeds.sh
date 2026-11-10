echo "== E5 invocations (status lines)"; grep -h "e5 gen6\|gen6b stopped\|gen6b discarded\|pilot2 start" ~/regicide-jobs/truth/status | cut -c1-140
echo "== seed lines in the logs"; for f in ~/regicide-jobs/truth/e5_gen6a_n50k_egdb.log ~/regicide-jobs/truth/e5_gen6b_n50k_egdb.log; do echo "$f: $(grep -c '^seeds:' $f) starts"; done
for f in ~/regicide-data/truth/gen6a_n50k_egdb.bin ~/regicide-data/truth/gen6b_n50k_egdb.bin; do s=$(stat -c %s $f); echo "$(basename $f) bytes $s mod16 $((s % 16)) records $((s / 16))"; done
