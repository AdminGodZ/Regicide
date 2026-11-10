#!/usr/bin/env bash
# rs_threads.sh N: restart the (resumable) rescore chain with N threads. It continues every file where it stopped
# (rescore resumes after the last complete record of its .part output).
source /mnt/c/regicide-match/jobs/truth/common.sh
for p in $(pgrep -f "regicide-jobs/truth/rescore_chain.sh"); do kill $p; done
pkill -CONT -x rescore-tr1; pkill -x rescore-tr1; sleep 2
st "rescore_chain restarting with $1 threads"
RS_THREADS=$1 bash /mnt/c/regicide-match/jobs/truth/launch.sh rescore_chain.sh
