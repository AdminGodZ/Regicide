#!/bin/bash
# netpost.sh TAG LABEL DB: appends the result row to RESULTS.md, commits the per-game files, pushes.
N=~/regicide-jobs/netm; W=~/Documents/Projects/Regicide-searchlab; tag=$1; label=$2; db=$3; out=$N/matches/$tag
dst=$W/experiments/search-lab/netm/matches/$tag; mkdir -p $dst
for c in a b c d e f; do
  mkdir -p $dst/$c; cp $out/$c/results.csv $out/$c/summary.txt $dst/$c/; gzip -c $out/$c/games.pdn > $dst/$c/games.pdn.gz
done
cp $out/pooled.txt $out/stats.txt $out/validate.txt $dst/
row=$(python3 -I - "$tag" "$label" "$db" <<'PY'
import re, sys
tag, label, db = sys.argv[1:4]
d = '/home/admingod/regicide-jobs/netm/matches/' + tag
p = open(d + '/pooled.txt').read(); s = open(d + '/stats.txt').read()
wdl = re.search(r'W/D/L (\d+)/(\d+)/(\d+)', p).groups(); pent = re.search(r'= (\[.*?\])', p).group(1)
elo = re.search(r'Elo ([+-][\d.]+) \[([+-][\d.]+), ([+-][\d.]+)\].*LOS ([\d.]+)%', p).groups()
A = re.search(r'A: moves \d+ mean ([\d.]+) s/move, mean depth first 20 searched moves ([\d.]+)', s).groups()
B = re.search(r'B: moves \d+ mean ([\d.]+) s/move, mean depth first 20 searched moves ([\d.]+)', s).groups()
ff = re.search(r'forfeits/illegal: (.*)', s).group(1)
print(f'| {label} | {db} | {wdl[0]} / {wdl[1]} / {wdl[2]} | {pent} | **{elo[0]} [{elo[1]}, {elo[2]}]** | {elo[3]}% | '
      f'{A[0]} / {B[0]} s | {A[1]} / {B[1]} | {ff} |')
PY
)
echo "$row" >> $W/experiments/search-lab/RESULTS.md
cp $N/runner.log $N/queue.txt $W/experiments/search-lab/netm/
cd $W && git add -A experiments/search-lab && git commit -qm "search-lab net match $label: $(tail -1 $out/pooled.txt | cut -d' ' -f1-5)

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>" && git push -q 2>&1 | tail -1
echo "$row"
