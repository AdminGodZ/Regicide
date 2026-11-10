#!/usr/bin/env bash
L=~/regicide-jobs/booklab
until grep -q "^F4 done" $L/status; do sleep 30; done
bash /mnt/c/regicide-match/jobs/booklab/wp10_blocks.sh 3 4 5
