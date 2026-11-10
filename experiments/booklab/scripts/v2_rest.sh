#!/usr/bin/env bash
# Rest of validation round 2 (bl-r5) and the Skull-Cracker re-test (bl-r6), after WP9 step 3.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
until grep -q "WP9 wp9conv1 done" $L/status; do sleep 30; done
echo "WP6 v2 rest start $(date -Is)" >> $L/status
for t in cake-b-h1 guinn-b-h1 cake-b-h2 kingsrow-b-h2 guinn-b-h2; do
  opp=${t%%-*}; h=${t##*-}
  bash $J/bl_run.sh regicide:bl-r5 $opp v2-$t $J/acf156_$h.txt < /dev/null
done
bash $J/bl_target.sh bl-r6 t6-r6-guinn guinn 2 "10-15 22-17 15-19"
echo "VALIDATION v2 done $(date -Is)" >> $L/status
sleep 3
