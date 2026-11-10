W=~/regicide-truth-work/matches
grep -h "^A:\|pentanomial \[\|^Elo (pent" $W/n8bn2-vs-n6c/summary.txt
~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/pool_dirs.py $W/n8bn-vs-n6c $W/n8bn2-vs-n6c
