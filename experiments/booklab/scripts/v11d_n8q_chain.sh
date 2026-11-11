#!/usr/bin/env bash
# After the r11d n6c validation (lead 2026-11-11 07:30):
#   1. r11d with n8q, DLL bl-5c433dc-r11d from master 5c433dc (n8q is the built-in net there): KingsRow mode 3 runs a, b;
#      Cake a, b; GuiNN a; final-validation runner; tags v11d-n8q-<opp>-<run>-<half>;
#   2. census against Cake for the WP12 pilot: the production deck four more times with the same DLL (tags
#      c4-cake-<1..4>-<half>).
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "V11D done\|V11D STOPPED" $L/status; do sleep 60; done
st "V11D validation n8q start (bl-5c433dc-r11d)"
for spec in "kingsrow a" "kingsrow b" "cake a" "cake b" "guinn a"; do
  set -- $spec
  for h in h1 h2; do
    bash $J/bl_run_x.sh regicide:bl-5c433dc-r11d $1 v11d-n8q-$1-$2-$h $J/acf156_$h.txt < /dev/null \
      || { st "V11D STOPPED at v11d-n8q-$1-$2-$h"; exit 1; }
  done
done
st "V11D n8q done"
for i in 1 2 3 4; do for h in h1 h2; do
  bash $J/bl_run_x.sh regicide:bl-5c433dc-r11d cake c4-cake-$i-$h $J/acf156_$h.txt < /dev/null || { st "C4 STOPPED at c4-cake-$i-$h"; exit 1; }
done; done
st "C4 census done"
