for p in $(pgrep -f "regicide-jobs/truth/e5_gen.sh"); do kill $p; done
pkill -CONT -x datagen-tr5; pkill -CONT -x datagen-tr6; pkill -x datagen-tr5; pkill -x datagen-tr6; sleep 3
for f in ~/regicide-data/truth/gen6a_n50k_egdb.bin ~/regicide-data/truth/gen6b_n50k_egdb.bin; do s=$(stat -c %s $f); echo "$(basename $f) bytes $s mod16 $((s % 16))"; done
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 614 6
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 615 6
sleep 5; grep "e5 gen6" ~/regicide-jobs/truth/status | tail -n 2 | cut -c1-160; grep appending ~/regicide-jobs/truth/e5_gen6a_n50k_egdb.log | tail -n 1
