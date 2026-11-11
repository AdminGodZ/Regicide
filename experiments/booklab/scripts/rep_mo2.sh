R=~/regicide-match/results; L=~/regicide-jobs/booklab; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab; J=/mnt/c/regicide-match/jobs/booklab
mkdir -p $L/mo2; cd $R
for o in kingsrow cake guinn; do
  echo "== $o: $(grep '^A:' bl-mo2-$o/summary.txt | cut -c1-40); $(grep -h 'Unfinished\|^Terminations' bl-mo2-$o/summary.txt | tr '\n' ' ')"
  python3 $J/xdec.py bl-mo2-$o | grep -v "^  "
done | tee $L/mo2/decisive.txt
for spec in "ACF #16 Wilderness II|41|kingsrow" "ACF #43|23|kingsrow" "ACF #71 Diabolical Denny|25|kingsrow" "ACF #134|37|kingsrow" "ACF #6 Inferno|50|kingsrow" "ACF #163|15 19 33|cake"; do
  IFS='|' read op pl opp <<< "$spec"
  col=W; [ "$op" = "ACF #6 Inferno" ] && col=B
  python3 $S/e2_report.py --results $R --opening "$op" --colour $col --plies $pl --runs bl-mo2-$opp | sed -n '5p'
done | tee $L/mo2/known.md
python3 $S/claims_report.py bl-mo2-kingsrow bl-mo2-cake bl-mo2-guinn --md $L/mo2/claims.md | grep -E "^## |disagrees|First claim of"
python3 $S/clock_report.py bl-mo2-kingsrow bl-mo2-cake bl-mo2-guinn --md $L/mo2/clock.md
