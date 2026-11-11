R=~/regicide-match/results; L=~/regicide-jobs/booklab; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab; J=/mnt/c/regicide-match/jobs/booklab
mkdir -p $L/mo1; cd $R
for o in kingsrow cake guinn; do echo "== $o: $(grep '^A:' bl-mo1-$o/summary.txt | cut -c1-40)"; python3 $J/xdec.py bl-mo1-$o | grep -v "^  moves"; done > $L/mo1/decisive.txt
cat $L/mo1/decisive.txt
for spec in "ACF #16 Wilderness II|41|kingsrow" "ACF #43|23|kingsrow" "ACF #71 Diabolical Denny|25|kingsrow" "ACF #134|37|kingsrow" "ACF #163|15 19 33|cake"; do
  IFS='|' read op pl opp <<< "$spec"
  python3 $S/e2_report.py --results $R --opening "$op" --colour W --plies $pl --runs bl-mo1-$opp | sed -n '5p'
done | tee $L/mo1/known.md
python3 $S/clock_report.py bl-mo1-kingsrow bl-mo1-cake bl-mo1-guinn bl-mo-kingsrow-a-h1 bl-mo-cake-a-h1 bl-mo-guinn-a-h1 --md $L/mo1/clock.md
