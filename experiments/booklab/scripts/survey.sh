#!/usr/bin/env bash
# list all games.pdn files with game counts and engine names
for f in $(find ~/regicide-match/results ~/regicide-jobs ~/regicide-nnue*-work ~/regicide-*-work -name games.pdn 2>/dev/null | sort -u); do
  n=$(grep -c '^\[Event' $f)
  names=$(grep -h '^\[Black\|^\[White' $f | sed 's/.*"\(.*\)".*/\1/' | sed 's/\[[0-9]\]//' | sort -u | paste -sd'/' | cut -c1-80)
  echo "$n $f $names"
done
