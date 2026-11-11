#!/bin/bash
# waitfor.sh TAG [SECONDS]: waits (file reads only) until the current runner2 run logs DONE/FAILED for TAG.
N=~/regicide-jobs/netm
cur() { awk '/^RUNNER2 START/{buf=""} {buf=buf $0 "\n"} END{printf "%s", buf}' $N/runner.log; }
timeout ${2:-590} bash -c "until awk '/^RUNNER2 START/{f=0} {print}' $N/runner.log | tac | awk '/^RUNNER2 START/{exit} {print}' | /usr/bin/grep -qE '^(DONE|FAILED) $1'; do sleep 30; done"
cat $N/matches/$1*/?/results.csv 2>/dev/null | /usr/bin/grep -vc '^game'
tac $N/runner.log | awk '/^RUNNER2 START/{exit} {print}' | /usr/bin/grep -E "^(DONE|FAILED|INCOMPLETE) $1" | cut -c1-200
