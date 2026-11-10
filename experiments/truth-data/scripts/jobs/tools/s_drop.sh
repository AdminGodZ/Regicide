ls -la ~/regicide-data/truth/*.part 2>/dev/null | awk '{print $5, $9}'
pgrep -af "rescore_chain|rescore-tr" | cut -c1-80
rm -f ~/regicide-data/truth/gen5c_n50k_e3.bin.part
echo "TR rescore of the remaining old files dropped for good (n8c vs n8bn at 0.5 s -1.7 [-5.4, +1.9], laptop); partial gen5c_e3 output deleted $(date -Is)" | tee -a ~/regicide-jobs/r4/status >> ~/regicide-jobs/truth/status
date; nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/unique_count.py ~/regicide-data/truth/gen6a_n50k_egdb.bin ~/regicide-data/truth/gen6b_n50k_egdb.bin
grep "e5 gen6" ~/regicide-jobs/truth/status | tail -n 2 | cut -c1-150
df -h ~ | tail -1
