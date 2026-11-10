#!/bin/bash
# post.sh <testname> <label> <commit message>: appends the row to RESULTS.md, copies the log, commits and pushes.
J=~/regicide-jobs/searchlab; W=~/Documents/Projects/Regicide-searchlab
row=$(python3 -I $J/row.py "$1" | sed "s/^| $1 /| $2 /")
echo "$row" >> $W/experiments/search-lab/RESULTS.md
cp $J/logs/$1.log $W/experiments/search-lab/logs/matches/; cp $J/runner.log $J/queue.txt $W/experiments/search-lab/logs/
cd $W && git add -A experiments/search-lab && git commit -qm "$3

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>" && git push -q 2>&1 | tail -1
echo "$row"
