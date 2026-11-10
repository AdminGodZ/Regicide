#!/usr/bin/env bash
# Rest of WP9 step 4 (bl-conv3 / bl-m11e, skip-safe: finished halves are skipped), then the FINAL VALIDATION with
# bl-final1 (master 2efe11a + round-8 book): (a) the same 100 missed-win positions as bl-conv3, (b) two full acf156
# runs against each of Cake, KingsRow and GuiNN, by halves. Replaces the rest of validation round 2.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
for h in h1 h2; do for opp in cake kingsrow guinn; do for d in conv3 m11e; do
  bash $J/bl_run.sh regicide:bl-$d $opp s4-$d-$opp-$h $J/acf156_$h.txt < /dev/null
done; done; done
echo "WP9 step4 done $(date -Is)" >> $L/status
bash $J/wp9_run.sh bl-final1 wp9final1 $L/wp9/conv3_100
echo "FINAL1 start $(date -Is)" >> $L/status
for run in a b; do for h in h1 h2; do for opp in cake kingsrow guinn; do
  bash $J/bl_run.sh regicide:bl-final1 $opp f1-$opp-$run-$h $J/acf156_$h.txt < /dev/null
done; done; done
echo "FINAL1 done $(date -Is)" >> $L/status
sleep 3
