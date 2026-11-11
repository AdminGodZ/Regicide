R=~/regicide-match/results; L=~/regicide-jobs/booklab; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab; O=$L/e2d/report; mkdir -p $O
python3 $S/e2_report.py --results $R --opening "ACF #163" --colour W --plies 15 19 33 37 --runs bl-e2a-fresh163 bl-e2b-kept163 bl-e2d-cake163-natural --title "Cake, ACF #163, Regicide White" --md $O/cake163.md > /dev/null
python3 $S/e2_report.py --results $R --opening "ACF #71 Diabolical Denny" --colour W --plies 21 25 --runs bl-e2d-kr71-fresh bl-e2d-kr71-kept bl-e2d-kr71-natural --title "KingsRow mode 3, ACF #71, Regicide White" --md $O/kr71.md > /dev/null
python3 $S/e2_report.py --results $R --opening "ACF #43" --colour W --plies 23 --runs bl-e2d-kr43-fresh bl-e2d-kr43-kept bl-e2d-kr43-natural --title "KingsRow mode 3, ACF #43, Regicide White" --md $O/kr43.md > /dev/null
python3 $S/e2_report.py --results $R --opening "ACF #134" --colour W --plies 37 --runs bl-e2d-kr134-fresh bl-e2d-kr134-kept bl-e2d-kr134-natural --title "KingsRow mode 3, ACF #134, Regicide White" --md $O/kr134.md > /dev/null
python3 $S/e2_report.py --results $R --opening "ACF #16 Wilderness II" --colour W --plies 41 --runs bl-e2d-kr16-fresh bl-e2d-kr16-kept bl-e2d-kr16-natural --title "KingsRow mode 3, ACF #16, Regicide White" --md $O/kr16.md > /dev/null
for f in cake163 kr71 kr43 kr134 kr16; do echo "== $f"; sed -n '/^| Run | W/,/^$/p' $O/$f.md; done
echo "== Black games of the natural pairs"
for r in e2d-cake163-natural e2d-kr71-natural e2d-kr43-natural e2d-kr134-natural e2d-kr16-natural; do grep -A1 "A as black" $R/bl-$r/summary.txt | head -1 | sed "s/^/$r: /"; done
