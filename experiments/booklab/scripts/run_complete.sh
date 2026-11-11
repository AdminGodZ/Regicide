#!/usr/bin/env bash
# run_complete.sh RUNDIR: exit 0 if the run is complete, else 1 (the WP12 context guard, used by wp12_chain.sh and
# c2_chain.sh, tested by t_guard2.sh). Complete = every scheduled game was played (full decks, both colours; games
# ended by --cb-move-limit count as played: "Unfinished (move limit): K") AND exit_status is 0 where it exists; runs
# without exit_status (the final validation bl-f1-*) by the summary only.
d=$1
[ -f "$d/summary.txt" ] || exit 1
{ [ ! -f "$d/exit_status" ] || [ "$(cat "$d/exit_status")" = 0 ]; } || exit 1
read g t < <(sed -n 's/^Games: \([0-9]*\) of \([0-9]*\) scheduled.*/\1 \2/p' "$d/summary.txt")
u=$(sed -n 's/^Unfinished (move limit): \([0-9]*\).*/\1/p' "$d/summary.txt")
[ -n "$g" ] && [ -n "$t" ] && [ $((g + ${u:-0})) -eq "$t" ]
