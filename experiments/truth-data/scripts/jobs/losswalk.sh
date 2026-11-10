#!/usr/bin/env bash
# Phase 3, item 4b: where were the book-off games against Cake and GuiNN lost? truth losswalk with 10M nodes per position
# and the 8-piece database, 2 threads (truth-tr4: stopped by pausekeeper_truth under matchlock).
source /mnt/c/regicide-match/jobs/truth/common.sh
R=~/regicide-match/results; cp ~/regicide-truth/build/truth $J/bin/truth-tr4
st "losswalk start"
for d in bl-f4-cake-h1 bl-f4-cake-h2 pt-s2-a1-cake pt-s2-a2-cake pt-s2-b1-cake pt-s2-b2-cake pt-s2-c1-cake pt-s2-c2-cake tr-n8bn-cake1 tr-n8bn-cake2 \
         tr-n8bn-cake-c0dde2e tr-n8c-cake-c0dde2e tr-n6c-cake-c0dde2e tr-n6c-guinn-c0dde2e tr-n8bn-guinn-c0dde2e \
         tr-n8c-guinn-c0dde2e ba-deep-off-cake ba-deep-off2-cake ba-deep-off3-cake ba-off-769-cake ba-off-769b-cake \
         ba-off-769-2s-cake ba-tm-off-cake ba-deep-off-guinn ba-off-769-guinn; do
  [ -s $J/lw_$d.tsv ] && continue
  $J/bin/truth-tr4 losswalk --egdb ~/regicide-data/egdb --nodes 10000000 --threads 2 --out $J/lw_$d.tsv.tmp $R/$d/games.pdn >> $J/losswalk.log 2>&1 \
    && mv $J/lw_$d.tsv.tmp $J/lw_$d.tsv
  st "losswalk $d: $(( $(wc -l < $J/lw_$d.tsv) - 1 )) lost games"
done
st "LOSSWALK_DONE"
