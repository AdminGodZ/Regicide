#!/usr/bin/env bash
# Book r11c (lead 2026-11-11 04:30): as r11b (same games, same rule) except that a lost game counts against a move only
# if it is still playable under r10k5 (no later own position where r10k5 plays another move). Then: pins that differ
# from r11b, and the evidence of both candidate moves at #71 ply 9 and #34 ply 13.
set -e
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab; J=/mnt/c/regicide-match/jobs/booklab
cp $J/src/tools/booklab/pins.py $W/tools/booklab/
cd $L
RUNS="$(ls -d $R/bl-f1-cake-* $R/bl-f1-guinn-* $R/bl-f1-kingsrow-* $R/bl-k3-kingsrow-* $R/bl-v13-kingsrow-* $R/bl-v13k4-* $R/bl-t1a-60-*)"
python3 $W/tools/booklab/pins.py --runs $RUNS --book book_r10k5.bin --pinwalk $W/build-linux/booklab_pinwalk \
  --egdb ~/regicide-data/egdb --out pins_r11c.txt --report pins_r11c.md > pins_r11c.log 2>&1
sed -n 3,10p pins_r11c.md
{ cat decisions_r10k5.txt; echo; echo "# === WP11 pins on r10k5 (pins_r11c.txt; losses repaired downstream not counted)"; cat pins_r11c.txt; } > decisions_r11c.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11c.txt --out book_r11c.bin --report book_r11c.report.txt | tail -1
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11c.txt --out /tmp/r11c2.bin > /dev/null
cmp book_r11c.bin /tmp/r11c2.bin && echo "r11c rebuilds byte-identically"
sha256sum book_r11c.bin; grep -vc '^#\|^$' decisions_r11c.txt
python3 - <<'PY'
L = '/home/admingod/regicide-jobs/booklab'
def load(p):
    return {l.split(' | ')[0]: (l.split(' | ')[1], l.split(' | ')[3].strip()) for l in open(p) if ' | ' in l and not l.startswith('#')}
b, c = load(f'{L}/pins_r11b.txt'), load(f'{L}/pins_r11c.txt')
only_b = [k for k in b if k not in c]; only_c = [k for k in c if k not in b]; diff = [k for k in b if k in c and b[k][0] != c[k][0]]
print(f"\npins r11b {len(b)}, r11c {len(c)}; only in r11b {len(only_b)}, only in r11c {len(only_c)}, different move {len(diff)}")
for k in diff:
    print(f"DIFF {k} | r11b {b[k][0]} | r11c {c[k][0]} | {c[k][1][:110]}")
for k in only_c[:40]:
    print(f"NEW  {k} | r11c {c[k][0]} | {c[k][1][:110]}")
for k in only_b[:40]:
    print(f"GONE {k} | r11b {b[k][0]} | {b[k][1][:110]}")
PY
