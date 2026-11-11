#!/usr/bin/env bash
# Experiment 2 (lead 2026-11-11 08:30): does Cake's play depend on what its process played before? No book change:
# r11d with n8q (DLL bl-ddd25fc-r11d, master ddd25fc), runner and host from ddd25fc (bin/match-ddd2,
# bin/ddd25fc/cbhost.exe), PDN comments on, concurrency 8 in every arm, set book 3 read back for both engines.
#   a. ACF #163 as White, 24 games, --fresh-engines (new processes for every game): e2a-fresh163
#   b. the same 24 games with the processes kept (as known163 was played): e2b-kept163
#   c. the production deck 6 times with processes kept and 6 times with --fresh-engines, alternating:
#      e2c-kept-<i>-<half>, e2c-fresh-<i>-<half>
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "V11D n8q done\|V11D STOPPED" $L/status; do sleep 60; done
export MATCH=$L/bin/match-ddd2 CBHOST=$L/bin/ddd2/cbhost.exe
B163=$(python3 -c "
import sys; sys.path.insert(0, '$W/tools/booklab')
from theory import parse_pdn, ballot_of
for t, m in parse_pdn(open('$R/bl-v13k4-cake-a-h2/games.pdn').read()):
    if t['Round'] == '69.2': print(ballot_of(t['Opening'])[1])")
: > $L/wp12/e2_163.txt; for i in $(seq 24); do echo "| $B163 | A=W" >> $L/wp12/e2_163.txt; done
st "E2 start (#163 deck: $B163)"
EXTRA_ARGS="--allow-rejected-options --fresh-engines" bash $J/bl_run_w.sh regicide:bl-ddd25fc-r11d cake e2a-fresh163 $L/wp12/e2_163.txt 0.5 8 < /dev/null || { st "E2 STOPPED at a"; exit 1; }
EXTRA_ARGS="--allow-rejected-options" bash $J/bl_run_w.sh regicide:bl-ddd25fc-r11d cake e2b-kept163 $L/wp12/e2_163.txt 0.5 8 < /dev/null || { st "E2 STOPPED at b"; exit 1; }
st "E2 a, b done"
for i in 1 2 3 4 5 6; do
  for arm in kept fresh; do
    X="--allow-rejected-options"; [ $arm = fresh ] && X="$X --fresh-engines"
    for h in h1 h2; do
      EXTRA_ARGS="$X" bash $J/bl_run_w.sh regicide:bl-ddd25fc-r11d cake e2c-$arm-$i-$h $J/acf156_$h.txt 0.5 8 < /dev/null \
        || { st "E2 STOPPED at e2c-$arm-$i-$h"; exit 1; }
    done
  done
done
st "E2 done"
