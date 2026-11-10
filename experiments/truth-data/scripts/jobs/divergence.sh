#!/usr/bin/env bash
source /mnt/c/regicide-match/jobs/truth/common.sh
touch $LOCKS/yield_truth
nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/divergence.py > $J/divergence.md 2> $J/divergence.err
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
st "divergence done: $(grep -c '^| ACF' $J/divergence.md) rows"
