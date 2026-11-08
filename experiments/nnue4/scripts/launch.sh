#!/usr/bin/env bash
# launch.sh SCRIPT: run /mnt/c/regicide-match/jobs/r4/SCRIPT detached, output to ~/regicide-jobs/r4/SCRIPT.out
J=~/regicide-jobs/r4; S=/mnt/c/regicide-match/jobs/r4/$1; shift
cp $S $J/
setsid nohup bash $J/$(basename $S) "$@" > $J/$(basename $S .sh).out 2>&1 < /dev/null &
sleep 3; echo "launched $(basename $S) pid $!"
