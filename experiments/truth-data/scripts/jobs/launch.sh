#!/usr/bin/env bash
# launch.sh SCRIPT [ARGS]: run /mnt/c/regicide-match/jobs/truth/SCRIPT detached; output ~/regicide-jobs/truth/SCRIPT.out
J=~/regicide-jobs/truth; mkdir -p $J; S=/mnt/c/regicide-match/jobs/truth/$1; shift
cp $S $J/
setsid nohup bash $J/$(basename $S) "$@" > $J/$(basename $S .sh).out 2>&1 < /dev/null &
sleep 3; echo "launched $(basename $S) pid $!"
