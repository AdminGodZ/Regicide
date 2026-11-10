touch ~/regicide-jobs/locks/pk_hold
bash /mnt/c/regicide-match/jobs/truth/launch.sh pausekeeper_truth.sh
export EXPECT_SHA=df0a5f5724f8c3ae
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 616 6
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 617 6
sleep 8
grep -E "^TR e5|^PKT" ~/regicide-jobs/r4/status | tail -n 3 | cut -c1-170
grep -h "appending" ~/regicide-jobs/truth/e5_gen6*_n50k_egdb.log | tail -n 2
ps -o pid,stat,comm -C datagen-tr5,datagen-tr6
