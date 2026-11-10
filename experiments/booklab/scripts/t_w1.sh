#!/usr/bin/env bash
# WP7c: recipe test with bl-w1 (r5 + all recipes): each recipe ballot played twice with both colours (4 games) against
# the engine the recipe was stolen from. Starts after validation round 2 has finished.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
until grep -q "VALIDATION v2 done" $L/status; do sleep 30; done
for y in cake kingsrow guinn; do
  D=$L/decks/t-w1-$y.txt; : > $D
  while read -r b; do echo "$b" >> $D; echo "$b" >> $D; done < $L/decks/w1-ballots-$y.txt
  bash $J/bl_run.sh regicide:bl-w1 $y t-w1-$y $D 0.5 8 < /dev/null
done
echo "WP7 t-w1 done $(date -Is)" >> $L/status
sleep 3
