#!/usr/bin/env bash
# Validation of r11d (lead 2026-11-11 07:00): book r11d = r10k5 + 53,050 pins, "wins first"; DLL from master a18042f
# (engine as 85a9fe8: r11c's master, PR #61 interface fix) with net n6c, default time setting, final-validation runner,
# set book 3 read back for every engine.
#   0. targeted test of the repaired lines on the new DLL (10 KingsRow mode 3, 4 Cake, 4 GuiNN per line; no loss allowed);
#   1. n6c: KingsRow mode 3 runs a, b; Cake a, b; GuiNN a (production deck, 312 games each).
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results
st() { echo "$* $(date -Is)" >> $L/status; }
DLLPREFIX=bl-a18042f NCAKE=4 bash $J/wp13_test.sh r11d 8 60,71,34,141,9,62,93,53 < /dev/null || { st "V11D STOPPED in test 8"; exit 1; }
n=$(for o in kingsrow cake guinn; do sed -n 's/^A: +[0-9]* =[0-9]* -\([0-9]*\).*/\1/p' $R/bl-w13-t8-r11d-$o/summary.txt; done | awk '{s += $1} END {print s + 0}')
[ "$n" = 0 ] || { st "V11D STOPPED: test 8 has $n losses"; exit 1; }
st "V11D validation n6c start (bl-a18042f-r11d)"
for spec in "kingsrow a" "kingsrow b" "cake a" "cake b" "guinn a"; do
  set -- $spec
  for h in h1 h2; do
    bash $J/bl_run_x.sh regicide:bl-a18042f-r11d $1 v11d-n6c-$1-$2-$h $J/acf156_$h.txt < /dev/null \
      || { st "V11D STOPPED at v11d-n6c-$1-$2-$h"; exit 1; }
  done
done
st "V11D done"
