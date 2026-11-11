#!/usr/bin/env bash
# New pins on top of r10k5 (lead 2026-11-11 02:00, CPU only): from the validated full runs with r10 (final validation,
# r8 = r10 on these games; and the KingsRow mode-3 runs k3), r10k (v13) and r10k4 (v13k4), against all three opponents,
# KingsRow mode 3 included. Rule: a move is pinned only if no game through that position with that move was lost.
# Then: for the losses of these runs (#62, #93 with r10k; the seven k3 losses; and #53, #55 with r10k4), how many of
# their positions before the losing move (the first own move with a search score below -100) are now pinned, and
# whether the pinned/book move there differs from the lost game's move (the book would have left the lost line).
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab
cp $S/pins.py $W/tools/booklab/
cd $L
RUNS="$(ls -d $R/bl-f1-cake-* $R/bl-f1-guinn-* $R/bl-f1-kingsrow-* $R/bl-k3-kingsrow-* $R/bl-v13-kingsrow-* $R/bl-v13k4-*)"
echo "$RUNS" | sed 's|.*/||' | tr '\n' ' '; echo
python3 $W/tools/booklab/pins.py --runs $RUNS --book book_r10k5.bin --pinwalk $W/build-linux/booklab_pinwalk \
  --egdb ~/regicide-data/egdb --out pins_r11n.txt --report pins_r11n.md > pins_r11n.log 2>&1 || { echo "pins.py failed"; tail pins_r11n.log; exit 1; }
head -12 pins_r11n.md
{ cat decisions_r10k5.txt; echo; echo "# === WP11 pins on r10k5 (pins_r11n.txt)"; cat pins_r11n.txt; } > decisions_r11n.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11n.txt --out book_r11n.bin --report book_r11n.report.txt | tail -1
sha256sum book_r11n.bin
python3 - <<'PY'
import sys, re, subprocess
sys.path.insert(0, '/home/admingod/regicide-booklab/tools/booklab')
from theory import parse_pdn, family, ballot_of
R = '/home/admingod/regicide-match/results'; L = '/home/admingod/regicide-jobs/booklab'
PW = '/home/admingod/regicide-booklab/build-linux/booklab_pinwalk'; EG = '/home/admingod/regicide-data/egdb'
pins = {l.split(' | ')[0]: l.split(' | ')[1] for l in open(f'{L}/pins_r11n.txt') if '|' in l and not l.startswith('#')}
runs = ['bl-k3-kingsrow-a-h1', 'bl-k3-kingsrow-a-h2', 'bl-k3-kingsrow-b-h1', 'bl-k3-kingsrow-b-h2',
        'bl-v13-kingsrow-b-h1', 'bl-v13-kingsrow-b-h2', 'bl-v13k4-kingsrow-a-h1', 'bl-v13k4-cake-a-h1']
print("\n| Lost game | Regicide | losing move (first own score < -100) | own positions before it | pinned (new) | pinned with a different move | first position where r11n (r10k5 + pins) plays differently |")
print("|---|---|---|---|---|---|---|")
for run in runs:
    for t, m in parse_pdn(open(f'{R}/{run}/games.pdn').read()):
        col = 'B' if family(t['Black']) == 'regicide' else 'W'
        if not ((t['Result'] == '1-0' and col == 'W') or (t['Result'] == '0-1' and col == 'B')): continue
        mv = [x for x, _ in m]
        drop = next((i for i, (x, c) in enumerate(m) if (i % 2 == 0) == (col == 'B') and (s := re.search(r'\bscore (-?\d+)', c or '')) and int(s.group(1)) < -100), len(mv))
        line = f"0\t\t{col}\t3\t{' '.join(mv)}\n"
        rows = subprocess.run([PW, f'{L}/book_r11n.bin', EG], input=line, capture_output=True, text=True).stdout.splitlines()
        before = [r.split('\t') for r in rows if int(r.split('\t')[1]) < drop]
        pinned = [r for r in before if r[2] in pins]
        diffpin = [r for r in pinned if pins[r[2]] != r[3]]
        first = next((f"ply {int(r[1]) + 1}: {r[5]} instead of {r[3]}" for r in before if r[4] == '1' and r[5] != '-' and r[5] != r[3]), 'none before the losing move')
        print(f"| {ballot_of(t['Opening'])[0]} ({run} {t['Round']}) | {col} | ply {drop + 1} | {len(before)} | {len(pinned)} | {len(diffpin)} | {first} |")
PY
