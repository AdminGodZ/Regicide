#!/usr/bin/env bash
# Game records of the tagged runs (Worker, EngineGames / EngineGamesA+B): experiment 2 (e2*) and the match-order runs
# (mo*), gzipped (gzip -n: reproducible), into experiments/booklab/tests/<run>/games.pdn.gz next to summary.txt and
# results.csv. Only finished runs (exit_status present). Usage: pub_pdn.sh [MSGLINE]  (commits and pushes via publish.sh)
J=/mnt/c/regicide-match/jobs/booklab; W=~/regicide-booklab; R=~/regicide-match/results; E=$W/experiments/booklab
n=0
# also the old-order r11d n8q validation (Review A round 4: old against match order, first different move)
for d in $R/bl-e2a-* $R/bl-e2b-* $R/bl-e2c-* $R/bl-e2d-* $R/bl-mo-* $R/bl-mo1-* $R/bl-v11d-n8q-*; do
  [ -f $d/exit_status ] && [ -f $d/games.pdn ] || continue
  t=$E/tests/$(basename ${d#*/bl-}); mkdir -p $t
  [ -f $t/games.pdn.gz ] && [ $t/games.pdn.gz -nt $d/games.pdn ] && continue
  gzip -n -c $d/games.pdn > $t/games.pdn.gz; cp $d/exit_status $t/; n=$((n + 1))
done
echo "$n game files added or refreshed; total size $(du -ch $E/tests/{e2,mo}*/games.pdn.gz 2>/dev/null | tail -1)"
printf "%s\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n" "${1:-booklab: game records (PDN, gzipped) of the experiment-2 and match-order runs}" > /tmp/bl_msg.txt
bash $J/publish.sh /tmp/bl_msg.txt 2>&1 | tail -1
