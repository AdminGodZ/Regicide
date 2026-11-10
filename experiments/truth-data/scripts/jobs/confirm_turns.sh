#!/usr/bin/env bash
source /mnt/c/regicide-match/jobs/truth/common.sh
touch $LOCKS/yield_truth
nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/confirm_turns.py > $J/confirm_turns.md 2> $J/confirm_turns.err
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
st "confirm turns done"
