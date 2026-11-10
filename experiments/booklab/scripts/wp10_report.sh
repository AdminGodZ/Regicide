#!/usr/bin/env bash
# WP10 report over all WP10 runs so far (+ the final validation for block-2 comparison); copies results to the repo
W=~/regicide-booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; E=$W/experiments/booklab/wp10
cp /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/wp10_report.py $W/tools/booklab/
mkdir -p $E/games
# completed runs only (summary with the A: line, not interrupted): every reported game has its record exported below
RUNS=$(cd $R && for r in bl-w10-* bl-f4-* bl-f1-* bl-v11-*; do
  [ -f $r/summary.txt ] && grep -q "^A:" $r/summary.txt && ! grep -q "interrupted\|stopped:" $r/summary.txt && echo $r
done 2>/dev/null)
python3 $W/tools/booklab/wp10_report.py --results $R --runs $RUNS --changed $L/changed_entries.txt --pinwalk $W/build-linux/booklab_pinwalk --book $L/book_r10.bin \
  --manifest $W/experiments/holdout/manifest.json --replay $W/build-linux/booklab_replay --scan $L/bin/blk-dbscan \
  --egdb ~/regicide-data/egdb --out-md $E/report.md --out-tsv $E/games.tsv --pair EB:O --pair EB:E --pair EBN:EB --pair "EBN:EB(final,r8)" --pair "EB11(n6c):EB(final,r8)" --pair "EB11(n8q):EBN" "$@"
for r in $RUNS; do
  [ -f $R/$r/summary.txt ] || continue
  mkdir -p $E/runs/$r; cp $R/$r/summary.txt $R/$r/results.csv $E/runs/$r/
  gzip -9 -c $R/$r/games.pdn > $E/games/$r.pdn.gz
done
cp $L/changed_positions.tsv $L/changed_entries.txt $E/
