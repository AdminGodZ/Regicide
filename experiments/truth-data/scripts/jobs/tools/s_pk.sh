cat ~/regicide-jobs/locks/bg_patterns | grep -v "^#"
pkill -f 'bash /home/admingod/regicide-jobs/truth/pausekeeper_truth.sh'; sleep 1
pkill -CONT -x datagen-tr5 2>/dev/null
bash /mnt/c/regicide-match/jobs/truth/launch.sh pausekeeper_truth.sh; sleep 8
grep PKT ~/regicide-jobs/r4/status | tail -n 2; ps -o pid,stat,comm -C datagen-tr5,datagen-tr6,blk-multiscore; cat /proc/loadavg
