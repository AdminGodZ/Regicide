#!/usr/bin/env bash
# WP10 configuration 4 on the production deck: master 456e9b2 + book_r10 + net n8q (set evalfile; sha256
# 681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24, experiments/truth-data/n8q.nnue on master c4d0ded),
# acf156 both colours, one 312-game run against each opponent, same runner and settings as the final validation.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
export EXTRA_A='set evalfile C:\regicide-match\nets\booklab\n8q.nnue'
echo "F4 start $(date -Is)" >> $L/status
for h in h1 h2; do for opp in cake kingsrow guinn; do
  bash $J/bl_run_x.sh regicide:bl-456e9b2-r10 $opp f4-$opp-$h $J/acf156_$h.txt < /dev/null
done; done
echo "F4 done $(date -Is)" >> $L/status
sleep 3
