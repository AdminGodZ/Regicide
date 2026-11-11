#!/usr/bin/env bash
# Smoke test of the T1 tooling: the logging runner and t1_report.py on 2 openings x 2 colours per arm and part.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
head -1 $L/wp10/parts/H3_0.txt > $L/t1s_H3_0.txt; head -1 $L/wp10/parts/H3_1.txt > $L/t1s_H3_1.txt
export MATCH=$L/bin/match-pr49-movelog EXTRA_ARGS=--allow-rejected-options CBHOST=$L/bin/pr49/cbhost.exe
for part in H3_0 H3_1; do for arm in 60 100; do
  EXTRA_A="set param tm_soft=$arm" bash $J/bl_run_w.sh regicide:bl-456e9b2-r10k2 cake t1z-$arm-cake-$part $L/t1s_$part.txt 0.5 4 < /dev/null
done; done
python3 $J/src/tools/booklab/t1_report.py --results ~/regicide-match/results --repeats z --boot 200 > $L/t1_smoke_report.md 2>&1
echo "T1 smoke done $(date -Is)" >> $L/status
