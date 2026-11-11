#!/usr/bin/env bash
# WP12 pilot (lead 2026-11-11; experiments/booklab/wp12/PROTOCOL.md). Baseline: book r11c in a DLL from master cc29fc9
# (bl-cc29fc9-r11c, net n6c); games with runner and host from cc29fc9 (bin/match-cc29, bin/cc29fc9/cbhost.exe), the
# boundary for the new runner; 0.5 s/move CheckerBoard banking, set book 3 for both engines, Cake 1.89f.
#   1. candidates (CPU, 6 threads, not timed): points from the r11c validation games against Cake with n6c
#      (bl-v11c-n6c-cake-{a,b}-{h1,h2}); contexts from every validated run; scoring 20M nodes; wp12/c1/
#   2. (after the r11c validation) the known point first: ACF #163 as White against Cake, 20 games with r11c
#      (Cake won nothing there; our first win against Cake was bl-v13k4-cake-a-h2 69.2): how often Cake repeats the line
#      of that game, how often we win;
#   3. discovery: the first 300 candidates, one game each (wp12_discover.py), ledger wp12/c1/disc.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab
S=$J/src/tools/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
for f in wp12_points.py wp12_select.py wp12_discover.py wp12_gates.py wp12_dll.sh; do cp $S/$f $W/tools/booklab/; done
# Points, contexts and selection only after EVERY validation run is complete (review 2026-11-11 finding 6: built after
# the first Cake half-run, c1 missed the contexts of games that finished later); the move scores stay cached.
until grep -q "V11C done\|V11C STOPPED" $L/status; do sleep 60; done
grep -q "V11C STOPPED" $L/status && { st "W12 not started: the r11c validation stopped"; exit 1; }
for d in $R/bl-v11c-* $R/bl-v13k4-* $R/bl-v13-kingsrow-* $R/bl-k3-kingsrow-* $R/bl-f1-*; do
  # exit_status 0; the final-validation runs (bl-f1-*) predate exit_status: their summary must say all games played
  [ "$(cat $d/exit_status 2>/dev/null)" = 0 ] || { [ ! -f $d/exit_status ] && grep -Eq "^Games: ([0-9]+) of \1 scheduled" $d/summary.txt; } \
    || { st "W12 not started: $d not complete"; exit 1; }
done
st "W12 candidates start"
mkdir -p $L/wp12/c1; cd $L/wp12/c1
python3 $W/tools/booklab/wp12_points.py --runs $R/bl-v11c-n6c-cake-a-h1 $R/bl-v11c-n6c-cake-a-h2 $R/bl-v11c-n6c-cake-b-h1 $R/bl-v11c-n6c-cake-b-h2 \
  --context-runs $(ls -d $R/bl-v11c-* $R/bl-v13k4-* $R/bl-v13-kingsrow-* $R/bl-k3-kingsrow-* $R/bl-f1-*) \
  --book $L/book_r11c.bin --pinwalk $W/build-linux/booklab_pinwalk --egdb ~/regicide-data/egdb --out points.tsv > points.log 2>&1 \
  || { st "W12 STOPPED: points"; exit 1; }
nice python3 $W/tools/booklab/wp12_select.py --points points.tsv --decisions $L/decisions_r10k5.txt --multiscore $L/bin/blk-multiscore \
  --net $W/src/weights/net.bin --egdb ~/regicide-data/egdb --book $L/book_r11c.bin --threads 6 --nodes 20000000 --slots 3 \
  --cache $L/wp12/cache.tsv --out $L/wp12/c1 > select.log 2>&1 || { st "W12 STOPPED: select"; exit 1; }
st "W12 candidates done: $(grep -vc '^#\|^rank' candidates.tsv) candidates ($(head -1 candidates.tsv))"
until grep -q "V11C done\|V11C STOPPED" $L/status; do sleep 60; done
grep -q "V11C STOPPED" $L/status && { st "W12 not started: the r11c validation stopped"; exit 1; }
# 2. the known point
B163=$(python3 -c "
import sys; sys.path.insert(0, '$W/tools/booklab')
from theory import parse_pdn, ballot_of
for t, m in parse_pdn(open('$R/bl-v13k4-cake-a-h2/games.pdn').read()):
    if t['Round'] == '69.2': print(ballot_of(t['Opening'])[1])")
: > $L/wp12/known163.txt; for i in $(seq 20); do echo "| $B163 | A=W" >> $L/wp12/known163.txt; done
MATCH=$L/bin/match-cc29 EXTRA_ARGS=--allow-rejected-options CBHOST=$L/bin/cc29/cbhost.exe \
  bash $J/bl_run_w.sh regicide:bl-cc29fc9-r11c cake w12-known163 $L/wp12/known163.txt 0.5 8 < /dev/null || { st "W12 STOPPED: known point"; exit 1; }
# 3. discovery
MATCH=$L/bin/match-cc29 CBHOST=$L/bin/cc29/cbhost.exe python3 $W/tools/booklab/wp12_discover.py --candidates $L/wp12/c1/candidates.tsv \
  --first 300 --out $L/wp12/c1/disc --base-dll bl-cc29fc9-r11c --base-book $L/book_r11c.bin > $L/wp12/c1/disc.log 2>&1 \
  || { st "W12 STOPPED: discovery"; exit 1; }
st "W12 discovery done"
