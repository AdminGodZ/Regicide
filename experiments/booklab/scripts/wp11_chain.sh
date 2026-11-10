#!/usr/bin/env bash
# Order: WP10 block 3 -> block 4 -> WP11 validation -> block 5 (block 6 decided later).
# When "WP10 block 4 done" appears, stop wp10_blocks.sh (it would start block 5) and any block-5 run that only waits
# for the lock, then play the WP11 validation: book r11 (r10 + pins) with n6c, then with n8q, one acf156 run per
# opponent, same runner (match-747ab51) and settings as the final validation, then restart block 5.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
until grep -q "WP10 block 4 done" $L/status; do sleep 20; done
for p in $(pgrep -f "[w]p10_blocks.sh|[w]p10_chain.sh"); do kill $p; done
for p in $(pgrep -f "[b]l_run_w.sh regicide:.* w10-b5-"); do
  if pgrep -P $p -x flock > /dev/null; then pkill -P $p; kill $p; fi
done
echo "WP11 validation start (block 5 paused) $(date -Is)" >> $L/status
for net in n6c n8q; do
  EA=""; [ $net = n8q ] && EA='set evalfile C:\regicide-match\nets\booklab\n8q.nnue'
  for h in h1 h2; do for opp in cake kingsrow guinn; do
    EXTRA_A="$EA" bash $J/bl_run_x.sh regicide:bl-456e9b2-r11 $opp v11-$net-$opp-$h $J/acf156_$h.txt < /dev/null
  done; done
  echo "WP11 validation $net done $(date -Is)" >> $L/status
done
bash $J/wp10_blocks.sh 5
sleep 3
