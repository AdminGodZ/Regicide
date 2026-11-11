#!/usr/bin/env bash
# T1 time allocation (experiments/time-policy/README.md on master e670910): tm_soft 60 (default, sent explicitly)
# against 100. Master engine + book r11 + net n6c (bl-456e9b2-r11), one thread, hash 128, book mode 3 for both
# engines (read back), Cake on holdout panel H3 (156 openings x both colours = 312 games per arm), the PR #49 runner
# (bin/match-pr49) and host (bin/pr49-c4fb8fd/cbhost.exe) for both arms. Arms alternate by part (H3_0: 60 then 100;
# H3_1: 100 then 60). The tm_soft option is read back from the engine's reply before each run (bl_run_w.sh).
# Runner: bin/match-pr49-movelog = the PR #49 runner plus a per-move log (games/<n>.pdn.moves.tsv: the runner's own
# bank before the move, offered maxtime, wall, host and charged time; patch tools/booklab/patches/match_movelog.diff).
#   t1_run.sh [REPEAT]   (REPEAT: a/b..., for a second run of both arms)
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
R=${1:-a}
export MATCH=$L/bin/match-pr49-movelog EXTRA_ARGS=--allow-rejected-options CBHOST=$L/bin/pr49/cbhost.exe
st() { echo "$* $(date -Is)" >> $L/status; }
st "T1 start (repeat $R)"
for part in H3_0 H3_1; do
  case $part in H3_0) arms="60 100" ;; H3_1) arms="100 60" ;; esac
  for arm in $arms; do
    EXTRA_A="set param tm_soft=$arm" \
      bash $J/bl_run_w.sh regicide:${T1DLL:-bl-456e9b2-r11} cake t1$R-$arm-cake-$part $L/wp10/parts/$part.txt 0.5 8 < /dev/null \
      || { st "T1 STOPPED at t1$R-$arm-cake-$part"; exit 1; }
  done
done
st "T1 done (repeat $R)"
