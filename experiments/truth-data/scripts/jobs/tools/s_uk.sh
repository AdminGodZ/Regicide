D=/home/admingod/regicide-data
nice ~/regicide-nnue/.venv/bin/python /mnt/c/regicide-match/jobs/truth/tools/unique_keys.py "E5=$D/truth/gen6a_n50k_egdb.bin,$D/truth/gen6b_n50k_egdb.bin" "gen5a=$D/train/gen5a_n50k.bin" | tee ~/regicide-jobs/truth/unique_keys.txt
