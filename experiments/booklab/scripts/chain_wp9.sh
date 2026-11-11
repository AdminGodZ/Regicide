#!/usr/bin/env bash
# After WP9 step 3: step 4 (full acf156 runs vs Cake, KingsRow, GuiNN with bl-conv2 (origin/t3/conversion-merge 25fb1ed), and with bl-m11e = its master base 11e19b4 with the same
# engine base and book without the fix), interleaved by half; then the rest of validation round 2 and the r6 re-test.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
until grep -q "WP9 wp9conv1 done" $L/status; do sleep 30; done
echo "WP9 step4 start $(date -Is)" >> $L/status
for h in h1 h2; do for opp in cake kingsrow guinn; do for d in conv2 m11e; do
  bash $J/bl_run.sh regicide:bl-$d $opp s4-$d-$opp-$h $J/acf156_$h.txt < /dev/null
done; done; done
echo "WP9 step4 done $(date -Is)" >> $L/status
echo "WP6 v2 rest start $(date -Is)" >> $L/status
for t in cake-b-h1 guinn-b-h1 cake-b-h2 kingsrow-b-h2 guinn-b-h2; do
  opp=${t%%-*}; h=${t##*-}
  bash $J/bl_run.sh regicide:bl-r5 $opp v2-$t $J/acf156_$h.txt < /dev/null
done
bash $J/bl_target.sh bl-r6 t6-r6-guinn guinn 2 "10-15 22-17 15-19"
echo "VALIDATION v2 done $(date -Is)" >> $L/status
sleep 3
