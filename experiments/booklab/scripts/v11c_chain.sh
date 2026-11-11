#!/usr/bin/env bash
# Validation of r11c (lead 2026-11-11 04:30): book r11c = r10k5 + 45,916 pins (losses repaired downstream not counted); DLLs from master 60e593e
# (bl-60e593e-r11c with n6c, bl-60e593e-r11c-n8q with n8q embedded from bd1c5c0), default time setting
# (tm_soft 60), final-validation runner match-747ab51, set book 3 read back for every engine.
#   0. targeted test of the repaired lines (ACF #60, #71, #34, #141, #9, #62, #93 Black; #53 White) with the n6c DLL,
#      as before the first runs on a new base (10 KingsRow mode 3, 4 Cake, 4 GuiNN per line);
#   1. n6c: KingsRow mode 3 runs a, b; Cake a, b; GuiNN a (production deck, 312 games each);
#   2. n8q: the same five runs.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results
st() { echo "$* $(date -Is)" >> $L/status; }
DLLPREFIX=bl-60e593e NCAKE=4 bash $J/wp13_test.sh r11c 7 60,71,34,141,9,62,93,53 < /dev/null || { st "V11C STOPPED in test 7"; exit 1; }
n=$(for o in kingsrow cake guinn; do sed -n 's/^A: +[0-9]* =[0-9]* -\([0-9]*\).*/\1/p' $R/bl-w13-t7-r11c-$o/summary.txt; done | awk '{s += $1} END {print s + 0}')
[ "$n" = 0 ] || { st "V11C STOPPED: test 7 has $n losses"; exit 1; }
for net in n6c n8q; do
  D=bl-60e593e-r11c; [ $net = n8q ] && D=bl-60e593e-r11c-n8q
  st "V11C validation $net start ($D)"
  for spec in "kingsrow a" "kingsrow b" "cake a" "cake b" "guinn a"; do
    set -- $spec
    for h in h1 h2; do
      bash $J/bl_run_x.sh regicide:$D $1 v11c-$net-$1-$2-$h $J/acf156_$h.txt < /dev/null \
        || { st "V11C STOPPED at v11c-$net-$1-$2-$h"; exit 1; }
    done
  done
  st "V11C validation $net done"
done
st "V11C done"
