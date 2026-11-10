kill 1852457 2097633 2>/dev/null; pkill -CONT -x datagen-tr6; kill 2510062 2>/dev/null; sleep 3
pgrep -af "datagen-tr6|after_gen4a" | cut -c1-80
f=~/regicide-data/truth/gen6a_n50k_egdb.bin; echo "gen6a now $(stat -c %s $f) bytes"
truncate -s 66707456 $f; echo "gen6a truncated to $(stat -c %s $f) bytes ($(( $(stat -c %s $f) / 16 )) records)"
echo "TR gen6a: a stray restart (old e5_gen.sh of after_gen4a, rewritten under it) appended with seed 611 for ~1 min; truncated back to 66707456 bytes $(date -Is)" | tee -a ~/regicide-jobs/r4/status >> ~/regicide-jobs/truth/status
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 614 6
sleep 6; grep "e5 gen6a" ~/regicide-jobs/truth/status | tail -n 1 | cut -c1-160; grep appending ~/regicide-jobs/truth/e5_gen6a_n50k_egdb.log | tail -n 1
