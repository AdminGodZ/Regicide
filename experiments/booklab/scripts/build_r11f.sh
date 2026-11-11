#!/usr/bin/env bash
# Book r11f (lead 2026-11-11 14:32): r11d + the #6 Black repair (8-11 instead of 6-10 at ply 17, reviewer B; accepted
# on the 100M scores) + pins from the 48 drawn KingsRow games of the WP14 item-0 test (w14c6-kingsrow-fresh/kept) by the
# usual rule (wins first, no move from a lost game), so that the new line's later positions (ply 25 and on) are book
# moves. Built from master's base book (book_master.bin, book_deep2b 3550e48) with booklab_apply, rebuilt twice
# (byte-identical check). Also counts r11d's book entries on the old 6-10 line that the change makes unreachable.
set -e
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab; S=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab
FEN="B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14"
cd $L; mkdir -p wp11f
# 1. r11d with the ply-17 pin replaced
python3 - "$FEN" <<'PY'
import sys
fen = sys.argv[1]
out, n = [], 0
for l in open('decisions_r11d.txt'):
    if l.startswith(fen + ' | '):
        n += 1
        l = (f"{fen} | 8-11 | force | WP14 #6 Black repair (reviewer B round 3: 8-11 at 400M root -10, forced 8-11 -19/-21 "
             f"against 6-10 -67; test w14c6: KingsRow 0/48/0, the new line's low points -2/-8/-23 at 100M; lead accepted "
             f"2026-11-11 14:32); replaces the r11d pin 6-10\n")
    out.append(l)
assert n == 1, n
open('wp11f/decisions_r11f0.txt', 'w').writelines(out)
PY
$W/build-linux/booklab_apply --book book_master.bin --decisions wp11f/decisions_r11f0.txt --out wp11f/book_r11f0.bin | tail -1
# 2. pins from the 48 drawn KingsRow games, against r11f0
python3 $S/pins.py --runs $R/bl-w14c6-kingsrow-fresh $R/bl-w14c6-kingsrow-kept --book wp11f/book_r11f0.bin \
  --pinwalk $W/build-linux/booklab_pinwalk --egdb ~/regicide-data/egdb --out wp11f/pins_r11f_new.txt --report wp11f/pins_r11f_new.md > wp11f/pins.log 2>&1
sed -n 3,12p wp11f/pins_r11f_new.md
{ cat wp11f/decisions_r11f0.txt; echo; echo "# === WP14 #6 repair: pins from the 48 drawn KingsRow games of w14c6 (pins_r11f_new.txt; wins first)"; cat wp11f/pins_r11f_new.txt; } > wp11f/decisions_r11f.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions wp11f/decisions_r11f.txt --out book_r11f.bin --report wp11f/book_r11f.report.txt | tail -1
$W/build-linux/booklab_apply --book book_master.bin --decisions wp11f/decisions_r11f.txt --out /tmp/r11f2.bin > /dev/null
cmp book_r11f.bin /tmp/r11f2.bin && echo "r11f rebuilds byte-identically"
echo "sha256 $(sha256sum book_r11f.bin | cut -c1-64)"
echo "decisions $(grep -vc '^#\|^$' wp11f/decisions_r11f.txt) (r11d $(grep -vc '^#\|^$' decisions_r11d.txt)); new pins $(grep -vc '^#\|^$' wp11f/pins_r11f_new.txt)"
# 3. the 48 games' own moves from ply 17 on: book moves under r11f until 8 pieces?
python3 - <<'PY'
import sys, subprocess, collections, os
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
R = os.path.expanduser('~/regicide-match/results'); L = os.path.expanduser('~/regicide-jobs/booklab')
inp, n = [], 0
for run in ('bl-w14c6-kingsrow-fresh', 'bl-w14c6-kingsrow-kept'):
    for t, m in parse_pdn(open(f'{R}/{run}/games.pdn').read()):
        inp.append(f"{n}\t\tB\t16\t{' '.join(x for x, _ in m)}\n"); n += 1
out = subprocess.run([os.path.expanduser('~/regicide-booklab/build-linux/booklab_pinwalk'), f'{L}/book_r11f.bin', os.path.expanduser('~/regicide-data/egdb')],
                     input=''.join(inp), capture_output=True, text=True).stdout
cov = collections.Counter()
for l in out.splitlines():
    f = l.split('\t')
    if f[1] == 'ERR': raise SystemExit(l)
    cov['eligible' if f[4] == '1' else 'king run'] += 1
    if f[4] == '1': cov['book = game move' if f[5] == f[3] else ('no book move' if f[5] == '-' else 'book differs')] += 1
print('own positions of the 48 games from ply 17 to 8 pieces under r11f:', dict(cov))
PY
# 4. r11d entries on the old 6-10 line that become unreachable: own positions after ply 17 of the recorded #6 Black games
#    through 6-10 that are book entries of r11d and occur in no recorded game that avoids 6-10 at ply 17
python3 - "$FEN" <<'PY'
import sys, glob, os, subprocess
sys.path.insert(0, '/mnt/c/regicide-match/jobs/booklab/src/tools/booklab')
from theory import parse_pdn
fen = sys.argv[1]; L = os.path.expanduser('~/regicide-jobs/booklab')
games = []
for d in glob.glob(os.path.expanduser('~/regicide-match/results/bl-*')):
    p = d + '/games.pdn'
    if not os.path.exists(p) or '.' in os.path.basename(d): continue
    for t, m in parse_pdn(open(p, errors='replace').read()):
        if t['Black'].lower().startswith('regicide') and not t.get('FEN'):
            games.append([x for x, _ in m])
inp = ''.join(f"{i}\t\tB\t0\t{' '.join(g)}\n" for i, g in enumerate(games))
out = subprocess.run([os.path.expanduser('~/regicide-booklab/build-linux/booklab_pinwalk'), f'{L}/book_r11d.bin'], input=inp, capture_output=True, text=True).stdout
old, other = set(), set()
through = {}
rows = [l.split('\t') for l in out.splitlines() if l.split('\t')[1] != 'ERR']
for f in rows:
    if f[2] == fen: through[int(f[0])] = f[3]
for f in rows:
    gi, ply = int(f[0]), int(f[1])
    if f[5] == '-' or f[4] != '1': continue
    if through.get(gi) == '6-10' and ply > 16: old.add(f[2])
    elif not (gi in through and ply > 16): other.add(f[2])
print(f"r11d book entries met only after 6-10 at ply 17 (now unreachable, kept in the file): {len(old - other)} "
      f"(of {len(old)} met on the old line; {len(old & other)} also reached by other games)")
PY
