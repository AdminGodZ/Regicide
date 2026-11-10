#!/usr/bin/env bash
# Verify that the DLL accepts "set evalfile <n8q>": master's runner stops at startup on a rejected option.
L=~/regicide-jobs/booklab; J=/mnt/c/regicide-match/jobs/booklab
cp ~/regicide-booklab/build-linux/tools/match $L/bin/match-master456
printf "9-13 22-18 10-15\n" > $L/decks/n8q_smoke.txt
for t in good bad; do
  P='C:\regicide-match\nets\booklab\n8q.nnue'; [ $t = bad ] && P='C:\regicide-match\nets\booklab\missing.nnue'
  MATCH=$L/bin/match-master456 EXTRA_A="set evalfile $P" bash $J/bl_run_x.sh regicide:bl-456e9b2-r10 guinn n8q-verify-$t $L/decks/n8q_smoke.txt 0.1 2 < /dev/null
  echo "== $t: $(head -3 $L/bl-n8q-verify-$t.log | tr '\n' ' ' | cut -c1-300)" >> $L/n8q_verify.txt
done
sleep 3
