#!/usr/bin/env bash
# Copies match summaries/CSVs, logs and nets into experiments/truth-data (what the lost assemble.sh did).
T=~/regicide-truth/experiments/truth-data; L=$T/logs; J=~/regicide-jobs/truth; W=~/regicide-truth-work
mkdir -p $L $T/matches
for d in $W/matches/*/ ~/regicide-nnue7-work/matches/*/ ~/regicide-match/results/tr-*/; do
  b=$(basename $d); mkdir -p $T/matches/$b; cp $d/summary.txt $d/results.csv $T/matches/$b/ 2>/dev/null; done
for r in pt-s2-a1-cake pt-s2-a2-cake; do
  m=$T/matches/ref-ponder-$r; mkdir -p $m; cp ~/regicide-match/results/$r/summary.txt ~/regicide-match/results/$r/results.csv $m/
  echo "Copied from ~/regicide-match/results/$r: a run by the ponder agent (jobs/ponder/run_p.sh, config A: DLL ponder-df7780a, built-in n6c, threads 1, ponder 0, book 0, acf156 x 2, 0.5 s/move). Used here as n6c's reference vs Cake." > $m/NOTE.txt
done
cp $J/p1_*_ballots.md $J/p1b_*_ballots.md $J/group_loss_*.md $J/e5_rate.txt $J/sentinel_check.txt $J/gen4a_rule_switch.txt $J/sanity_*.md $L/ 2>/dev/null
cp $W/logs/export_checks.txt $W/logs/export_rechecks.txt $W/logs/p5_speed.txt $L/ 2>/dev/null
for n in $W/runs/*.log; do cp $n $L/; done
for n in $W/*.nnue; do cp $n $T/; done
for n in $W/runs/*/config.json; do cp $n $T/config_$(basename $(dirname $n)).json; done
grep "^TR" ~/regicide-jobs/r4/status > $L/status.txt
cp /mnt/c/regicide-match/jobs/truth/*.sh /mnt/c/regicide-match/jobs/truth/*.py $T/scripts/jobs/ 2>/dev/null
mkdir -p $T/scripts/jobs/tools; cp /mnt/c/regicide-match/jobs/truth/tools/*.sh /mnt/c/regicide-match/jobs/truth/tools/*.py $T/scripts/jobs/tools/ 2>/dev/null
echo copied
