#!/usr/bin/env bash
# Build the theory table from every games.pdn on the machine (list regenerated each time).
cd ~/regicide-jobs/booklab
bash /mnt/c/regicide-match/jobs/booklab/survey.sh > pdn_list.txt
awk '$1>0' pdn_list.txt > pdn_list_nonempty.txt
nice python3 /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/theory.py build --db ${1:-theory.sqlite} \
  --replay ~/regicide-booklab/build-linux/booklab_replay --list pdn_list_nonempty.txt > theory_build.log 2>&1
echo "THEORY built $(tail -6 theory_build.log | head -1) $(date -Is)" >> status
