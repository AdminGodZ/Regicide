#!/usr/bin/env bash
# Book r11d (lead 2026-11-11 07:00): r10k5 + pins with "wins first" (a move from a game won against Cake or KingsRow is
# preferred to moves that only drew, by number of wins; GuiNN wins only where no candidate was played against Cake or
# KingsRow), avoided moves never pinned and avoided later own moves counted as downstream repairs, losses repaired
# downstream not counted; pool: the r11c pool (f1, k3, v13, v13k4, T1 arm 60) plus the r11c validation (v11c, both nets).
set -e
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab; J=/mnt/c/regicide-match/jobs/booklab
S=$J/src/tools/booklab
cp $S/pins.py $S/pinwalk.cpp $S/wins_vs_book.py $S/lost_vs_book.py $W/tools/booklab/
cd $L
RUNS="$(ls -d $R/bl-f1-cake-* $R/bl-f1-guinn-* $R/bl-f1-kingsrow-* $R/bl-k3-kingsrow-* $R/bl-v13-kingsrow-* $R/bl-v13k4-* $R/bl-t1a-60-* $R/bl-v11c-*)"
echo "runs: $(echo $RUNS | wc -w)"
python3 $W/tools/booklab/pins.py --runs $RUNS --book book_r10k5.bin --pinwalk $W/build-linux/booklab_pinwalk \
  --egdb ~/regicide-data/egdb --out pins_r11d.txt --report pins_r11d.md > pins_r11d.log 2>&1
sed -n 3,10p pins_r11d.md
{ cat decisions_r10k5.txt; echo; echo "# === WP11 pins on r10k5 (pins_r11d.txt; wins first)"; cat pins_r11d.txt; } > decisions_r11d.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11d.txt --out book_r11d.bin --report book_r11d.report.txt | tail -1
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11d.txt --out /tmp/r11d2.bin > /dev/null
cmp book_r11d.bin /tmp/r11d2.bin && echo "r11d rebuilds byte-identically"
sha256sum book_r11d.bin; grep -vc '^#\|^$' decisions_r11d.txt
python3 - <<'PY'
L = '/home/admingod/regicide-jobs/booklab'
def load(p):
    return {l.split(' | ')[0]: (l.split(' | ')[1], l.split(' | ')[3].strip()) for l in open(p) if ' | ' in l and not l.startswith('#')}
c, d = load(f'{L}/pins_r11c.txt'), load(f'{L}/pins_r11d.txt')
diff = [k for k in c if k in d and c[k][0] != d[k][0]]
print(f"\npins r11c {len(c)}, r11d {len(d)}; only in r11c {sum(1 for k in c if k not in d)}, only in r11d {sum(1 for k in d if k not in c)}, different move {len(diff)}")
with open(f'{L}/pins_r11d_vs_r11c.tsv', 'w') as fo:
    for k in diff:
        fo.write(f"{k}\t{c[k][0]}\t{d[k][0]}\t{d[k][1]}\n")
PY
