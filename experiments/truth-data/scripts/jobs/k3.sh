#!/usr/bin/env bash
source /mnt/c/regicide-match/jobs/truth/common.sh
touch $LOCKS/yield_truth
st "k3 analysis start"
nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/k3.py > $J/k3/k3.out 2> $J/k3/k3.err
rc=$?
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
st "k3 analysis end rc=$rc"
