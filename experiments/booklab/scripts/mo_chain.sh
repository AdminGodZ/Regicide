#!/usr/bin/env bash
# Match order (lead 2026-11-11): everything from master 1c6e997 (runner bin/match-1c6e with --pairs-together and the
# EngineGamesA / EngineGamesB tags, host bin/1c6e/cbhost.exe, DLL bl-1c6e997 = master as committed: built-in book r11d
# 7692986c, net n8q), set book 3 read back for both engines, PDN comments on, processes kept unless --fresh-engines.
# Starts after experiment 2's deck arm (E2 done).
#   2b: r11d validation in match order: --pairs-together, concurrency 8: KingsRow mode 3 a, b; Cake a, b; GuiNN a
#       (tags mo-<opp>-<run>-<half>)
#   2c: (own session, mo1_chain.sh) Cake, concurrency 1, pairs together, whole deck, no timing lock
#   2d: experiment 2 continued: Cake #163 natural pairs (24 pairs, our Black game before our White game); KingsRow
#       mode 3 #71, #43, #134, #16 as White: 24 games fresh, 24 kept, 24 natural pairs each
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "E2 done\|E2 STOPPED" $L/status; do sleep 60; done
export MATCH=$L/bin/match-1c6e CBHOST=$L/bin/1c6e/cbhost.exe
D=regicide:bl-1c6e997
st "MO start (runner $(sha256sum $MATCH | cut -c1-16), DLL $(sha256sum /mnt/c/regicide-match/engines/regicide/bl-1c6e997/regicide_cb.dll | cut -c1-16))"
for spec in "kingsrow a" "kingsrow b" "cake a" "cake b" "guinn a"; do
  set -- $spec
  for h in h1 h2; do
    EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh $D $1 mo-$1-$2-$h $J/acf156_$h.txt 0.5 8 < /dev/null \
      || { st "MO STOPPED at mo-$1-$2-$h"; exit 1; }
  done
done
st "MO 2b done"
mkdir -p $L/e2d
deck() {  # deck FILE MOVES SUFFIX: 24 lines
  : > $1; for i in $(seq 24); do echo "$2$3" >> $1; done
}
deck $L/e2d/n163.txt "12-16 21-17 16-20" ""
EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh $D cake e2d-cake163-natural $L/e2d/n163.txt 0.5 8 < /dev/null \
  || { st "MO STOPPED at e2d-cake163-natural"; exit 1; }
for o in "71:10-14 23-19 7-10" "43:9-14 22-18 11-16" "134:11-16 21-17 8-11" "16:9-13 23-18 11-16"; do
  n=${o%%:*}; mv=${o#*:}
  deck $L/e2d/w$n.txt "| $mv" " | A=W"
  deck $L/e2d/p$n.txt "$mv" ""
  EXTRA_ARGS="--allow-rejected-options --fresh-engines" bash $J/bl_run_w.sh $D kingsrow e2d-kr$n-fresh $L/e2d/w$n.txt 0.5 8 < /dev/null \
    || { st "MO STOPPED at e2d-kr$n-fresh"; exit 1; }
  EXTRA_ARGS="--allow-rejected-options" bash $J/bl_run_w.sh $D kingsrow e2d-kr$n-kept $L/e2d/w$n.txt 0.5 8 < /dev/null \
    || { st "MO STOPPED at e2d-kr$n-kept"; exit 1; }
  EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh $D kingsrow e2d-kr$n-natural $L/e2d/p$n.txt 0.5 8 < /dev/null \
    || { st "MO STOPPED at e2d-kr$n-natural"; exit 1; }
done
st "MO done"
