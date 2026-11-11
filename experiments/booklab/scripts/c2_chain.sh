#!/usr/bin/env bash
# WP12 pilot, Cake arm, campaign c2 (lead 2026-11-11): base = master 1c6e997 as committed (built-in book r11d, net n8q;
# DLL bl-1c6e997). Points from the match-order runs against Cake (mo-cake-{a,b}-{h1,h2}); contexts from every completed
# r11d n8q validation run (old order, v11d-n8q-*), the match-order runs (mo-*) and the concurrency-1 match-order runs
# (mo1-*); protected decisions = r10k5's; scoring with booklab_multiscore built on 1c6e997 (n8q), book off, 20 M
# nodes, 6 threads; scores cached by campaign key in wp12/cache.tsv. Starts only after experiment 2's deck arm and
# the three concurrency-1 runs have finished (no timed run of mine left), and only if every input run is complete.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab
S=$J/src/tools/booklab; B=~/regicide-match/builds/scorer-1c6e/src
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "^E2 done" $L/status && grep -q "^MO1 kingsrow" $L/status && grep -q "^MO1 cake" $L/status \
  && grep -q "^MO1 guinn" $L/status; do sleep 60; done
RUNS="$(ls -d $R/bl-mo-cake-a-h1 $R/bl-mo-cake-a-h2 $R/bl-mo-cake-b-h1 $R/bl-mo-cake-b-h2)"
CTX="$(ls -d $R/bl-v11d-n8q-* $R/bl-mo-* $R/bl-mo1-kingsrow $R/bl-mo1-cake $R/bl-mo1-guinn)"
for d in $CTX; do bash $J/run_complete.sh $d || { st "W12C2 not started: $d not complete"; exit 1; }; done
st "W12C2 points start ($(echo $CTX | wc -w) context runs)"
mkdir -p $L/wp12/c2; cd $L/wp12/c2
python3 $S/wp12_points.py --runs $RUNS --context-runs $CTX --book $L/book_r11d.bin --pinwalk $L/bin/blk-pinwalk-1c6e \
  --egdb ~/regicide-data/egdb --out points.tsv > points.log 2>&1 || { st "W12C2 STOPPED: points"; exit 1; }
st "W12C2 points done: $(($(wc -l < points.tsv) - 1)) points; scoring starts"
nice python3 $S/wp12_select.py --points points.tsv --decisions $L/decisions_r10k5.txt --multiscore $L/bin/blk-multiscore-1c6e \
  --net $B/src/weights/net.bin --egdb ~/regicide-data/egdb --book $L/book_r11d.bin --threads 6 --nodes 20000000 --slots 3 \
  --cache $L/wp12/cache.tsv --out $L/wp12/c2 > select.log 2>&1 || { st "W12C2 STOPPED: select"; exit 1; }
st "W12C2 candidates done: $(grep -vc '^#\|^rank' candidates.tsv) candidates ($(head -1 candidates.tsv))"
