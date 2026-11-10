#!/usr/bin/env bash
# WP6 validation: two full acf156 runs (each as two halves) against each of Cake, KingsRow and GuiNN.
#   v_run.sh DLLDIR ROUNDTAG
J=/mnt/c/regicide-match/jobs/booklab
DLL=$1; T=$2
H1=$J/acf156_h1.txt; H2=$J/acf156_h2.txt
echo "VALIDATION $T start dll $DLL $(date -Is)" >> ~/regicide-jobs/booklab/status
for run in a b; do
  for h in h1 h2; do
    for opp in cake kingsrow guinn; do
      D=$H1; [ $h = h2 ] && D=$H2
      bash $J/bl_run.sh "regicide:$DLL" $opp "$T-$opp-$run-$h" $D < /dev/null
    done
  done
done
echo "VALIDATION $T done $(date -Is)" >> ~/regicide-jobs/booklab/status
sleep 3
