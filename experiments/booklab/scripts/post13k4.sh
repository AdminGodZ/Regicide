#!/usr/bin/env bash
# WP13, fifth and sixth lines (#62, #93) and what follows (lead 2026-11-11 order), each step only if the previous one has no loss:
#   0. targeted test of r10k4 on ACF #93 (Regicide Black: 10 games against KingsRow mode 3, 4 against Cake, 4 GuiNN);
#   1. validation of r10k4: two production-deck runs against KingsRow mode 3, one each against Cake and GuiNN
#      (final-validation runner), tags v13k4-<opp>-<run>-<half>;
#   2. new pins from all validated games on top of r10k4 (final runs against Cake and GuiNN, the mode-1 KingsRow games,
#      the two r10k4 runs against KingsRow mode 3): book r11k = r10k4 + pins; DLL bl-456e9b2-r11k;
#   3. WP11 validation of r11k with n6c and n8q against all three (KingsRow mode 3), tags v11k-<net>-<opp>-<half>;
#   4. T1 with r11k.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; W=~/regicide-booklab
st() { echo "$* $(date -Is)" >> $L/status; }
losses() { for d in "$@"; do [ -f $d/summary.txt ] || { echo 999; continue; }
  sed -n 's/^A: +[0-9]* =[0-9]* -\([0-9]*\).*/\1/p' $d/summary.txt; done | awk '{s += $1} END {print s + 0}'; }
st "POST13K4 (fixed script) start"
bash $J/wp13_test.sh r10k4 4 93 < /dev/null || { st "POST13K4 STOPPED in test 4"; exit 1; }
n=$(losses $R/bl-w13-t4-r10k4-kingsrow $R/bl-w13-t4-r10k4-cake $R/bl-w13-t4-r10k4-guinn)
[ "$n" = 0 ] || { st "POST13K4 STOPPED: #93 test 4 has $n losses (try 4 or report)"; exit 1; }
st "WP13 validation (r10k4) start"
for spec in "kingsrow a" "kingsrow b" "cake a" "guinn a"; do
  set -- $spec
  for h in h1 h2; do
    bash $J/bl_run_x.sh regicide:bl-456e9b2-r10k4 $1 v13k4-$1-$2-$h $J/acf156_$h.txt < /dev/null \
      || { st "POST13K4 STOPPED at v13k4-$1-$2-$h"; exit 1; }
  done
done
V=$(for o in "kingsrow a" "kingsrow b" "cake a" "guinn a"; do set -- $o; echo $R/bl-v13k4-$1-$2-h1 $R/bl-v13k4-$1-$2-h2; done)
n=$(losses $V); [ "$n" = 0 ] || { st "POST13K4 STOPPED: the r10k4 validation has $n losses (lead decides)"; exit 1; }
st "WP13 validation (r10k4) done without loss"
cd $L
python3 $W/tools/booklab/pins.py --runs $R/bl-f1-cake-* $R/bl-f1-guinn-* $R/bl-f1-kingsrow-* $R/bl-v13k4-kingsrow-* \
  --book book_r10k4.bin --pinwalk $W/build-linux/booklab_pinwalk --egdb ~/regicide-data/egdb \
  --out pins_r11k.txt --report pins_r11k.md > pins_r11k.log 2>&1 || { st "POST13K4 STOPPED: pins.py failed"; exit 1; }
{ cat decisions_r10k4.txt; echo; echo "# === WP11 pins on r10k4 (pins_r11k.txt)"; cat pins_r11k.txt; } > decisions_r11k.txt
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11k.txt --out book_r11k.bin \
  --report book_r11k.report.txt > /dev/null || { st "POST13K4 STOPPED: booklab_apply failed"; exit 1; }
$W/build-linux/booklab_apply --book book_master.bin --decisions decisions_r11k.txt --out /tmp/r11k_b.bin > /dev/null
cmp -s book_r11k.bin /tmp/r11k_b.bin || { st "POST13K4 STOPPED: r11k does not rebuild identically"; exit 1; }
st "POST13K4 r11k built: $(grep -vc '^#\|^$' decisions_r11k.txt) decisions, sha256 $(sha256sum book_r11k.bin | cut -c1-64)"
bash $J/bl_build_dll.sh 456e9b2-r11k $L/book_r11k.bin 4 456e9b2 > $L/build_r11k.log 2>&1 || { st "POST13K4 STOPPED: DLL build"; exit 1; }
st "WP11 validation (r11k) start"
for net in n6c n8q; do
  EA=""; [ $net = n8q ] && EA='set evalfile C:\regicide-match\nets\booklab\n8q.nnue'
  for h in h1 h2; do for opp in cake kingsrow guinn; do
    EXTRA_A="$EA" bash $J/bl_run_x.sh regicide:bl-456e9b2-r11k $opp v11k-$net-$opp-$h $J/acf156_$h.txt < /dev/null \
      || { st "POST13K4 STOPPED at v11k-$net-$opp-$h"; exit 1; }
  done; done
  st "WP11 validation (r11k) $net done"
done
T1DLL=bl-456e9b2-r11k bash $J/t1_run.sh a < /dev/null || { st "POST13K4 STOPPED in T1"; exit 1; }
st "POST13K4 done"
