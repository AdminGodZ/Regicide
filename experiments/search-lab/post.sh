#!/bin/bash
# post.sh <testname> <label> <commit message>: appends the row to RESULTS.md, copies the log, commits and pushes.
# Nothing is appended, copied or committed unless row.py exits 0 and returns exactly one non-empty table row; a row
# labelled "(partial ...)" is refused unless the 4th argument is --partial (deliberate interim posts only).
J=~/regicide-jobs/searchlab; W=~/Documents/Projects/Regicide-searchlab
[ -s "$J/logs/$1.log" ] || { echo "post.sh: no log $J/logs/$1.log" >&2; exit 1; }
raw=$(python3 -I $J/row.py "$1"); rc=$?
[ $rc = 0 ] || { echo "post.sh: row.py failed (exit $rc) for $1" >&2; exit 1; }
[ "$(printf '%s\n' "$raw" | /usr/bin/grep -c '^| ')" = 1 ] && [ "$(printf '%s' "$raw" | wc -l)" = 0 ] \
  || { echo "post.sh: row.py did not return exactly one row for $1: '$raw'" >&2; exit 1; }
row=$(python3 -I -c 'import sys; n, l, r = sys.argv[1:4]; print(r.replace("| " + n + " ", "| " + l + " ", 1) if r.startswith("| " + n + " ") else "")' "$1" "$2" "$raw")
[ -n "$row" ] || { echo "post.sh: empty row" >&2; exit 1; }
case "$row" in *"(partial"*) [ "$4" = --partial ] || { echo "post.sh: $1 is partial; refusing without --partial: $row" >&2; exit 1; } ;; esac
echo "$row" >> $W/experiments/search-lab/RESULTS.md
cp $J/logs/$1.log $W/experiments/search-lab/logs/matches/; cp $J/runner.log $J/queue.txt $W/experiments/search-lab/logs/
cd $W && git add -A experiments/search-lab && git commit -qm "$3

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>" && git push -q 2>&1 | tail -1
echo "$row"
