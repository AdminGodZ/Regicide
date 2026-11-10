pgrep -af pausekeeper_truth | grep -v pgrep
for p in $(pgrep -f "regicide-jobs/truth/pausekeeper_truth.sh"); do kill $p; done; sleep 1
pkill -CONT -x datagen-tr5; pkill -CONT -x datagen-tr6
bash /mnt/c/regicide-match/jobs/truth/launch.sh pausekeeper_truth.sh; sleep 6
grep PKT ~/regicide-jobs/r4/status | tail -n 2; ps -o pid,stat,comm -C datagen-tr5,datagen-tr6
