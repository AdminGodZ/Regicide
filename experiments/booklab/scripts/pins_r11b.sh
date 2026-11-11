#!/usr/bin/env bash
# Book r11b (lead 2026-11-11 04:00): r10k5 + pins from the non-lost games (exactly one side Regicide, against Cake,
# KingsRow or GuiNN) of the validated runs with r10 (final validation bl-f1-*, KingsRow mode 3 bl-k3-*), r10k
# (bl-v13-kingsrow-*), r10k4 (bl-v13k4-*) and the T1 arm tm_soft 60 (bl-t1a-60-*). Rule: a move is pinned only if no
# game through that position with that move was lost. Then: reachability (witness games from a theory table of the
# same games), and for each lost game since r10 the move where it left the drawn games of its opening and colour.
set -e
L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab; J=/mnt/c/regicide-match/jobs/booklab
S=$J/src/tools/booklab
cp $S/pins.py $S/theory.py $S/reach_check.py $W/tools/booklab/
cd $L
RUNS="$(ls -d $R/bl-f1-cake-* $R/bl-f1-guinn-* $R/bl-f1-kingsrow-* $R/bl-k3-kingsrow-* $R/bl-v13-kingsrow-* $R/bl-v13k4-* $R/bl-t1a-60-*)"
echo "runs: $(echo $RUNS | wc -w): $(echo "$RUNS" | sed 's|.*/bl-||' | tr '\n' ' ')"
python3 $W/tools/booklab/pins.py --runs $RUNS --book book_r10k5.bin --pinwalk $W/build-linux/booklab_pinwalk \
  --egdb ~/regicide-data/egdb --out pins_r11b.txt --report pins_r11b.md > pins_r11b.log 2>&1
sed -n 1,9p pins_r11b.md
{ cat decisions_r10k5.txt; echo; echo "# === WP11 pins on r10k5 (pins_r11b.txt)"; cat pins_r11b.txt; } > decisions_r11b.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11b.txt --out book_r11b.bin --report book_r11b.report.txt | tail -1
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11b.txt --out /tmp/r11b2.bin > /dev/null
cmp book_r11b.bin /tmp/r11b2.bin && echo "r11b rebuilds byte-identically"
sha256sum book_r11b.bin
# reachability: theory table of the same games (witnesses) plus the main table
echo "$RUNS" | sed 's|$|/games.pdn|' > r11b_list.txt
rm -f theory_r11b.sqlite
python3 $W/tools/booklab/theory.py build --db theory_r11b.sqlite --replay $W/build-linux/booklab_replay --list r11b_list.txt > theory_r11b.log 2>&1
tail -2 theory_r11b.log
python3 $W/tools/booklab/reach_check.py --decisions decisions_r11b.txt --book book_r11b.bin --reach $W/build-linux/booklab_reach \
  --db theory_r11b.sqlite --limit 200000 --egdb ~/regicide-data/egdb > book_r11b.reach.txt 2>&1 || true
tail -1 book_r11b.reach.txt
