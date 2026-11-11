#!/usr/bin/env bash
# run_complete.sh RUNDIR: exit 0 if the run is complete, else 1 (the WP12 context guard, used by wp12_chain.sh and
# tested by t_guard2.sh). Complete = the summary says every scheduled game was played (full decks, both colours) AND
# exit_status is 0 where it exists; runs without exit_status (the final validation bl-f1-*) by the summary only.
d=$1
{ [ ! -f "$d/exit_status" ] || [ "$(cat "$d/exit_status")" = 0 ]; } && grep -Eq "^Games: ([0-9]+) of \1 scheduled" "$d/summary.txt" 2>/dev/null
