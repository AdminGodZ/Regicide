cd ~/regicide-match/results
grep "BL end tb6" ~/regicide-jobs/booklab/status | cut -c1-110
python3 /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/tb_compare.py --ref bl-mo1-kingsrow --reps $(ls -d bl-tb6-? 2>/dev/null) --md ~/regicide-jobs/booklab/cl/tb6.md
