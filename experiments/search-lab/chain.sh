#!/bin/bash
# Starts the LMR sweep (regicide-searchlab runner) once the net-match queue has finished M6a.
N=~/regicide-jobs/netm; J=~/regicide-jobs/searchlab
until grep -q '^M6a_' $N/done.txt; do sleep 60; done
touch $N/STOP; systemctl --user stop regicide-netm
rm -f $J/STOP
echo "CHAIN start sweep $(date -Is)" >> $J/runner.log
exec systemd-run --user --unit=regicide-searchlab --collect -p Nice=10 -p WorkingDirectory=$J \
  -p StandardOutput=append:$J/runner.log -p StandardError=append:$J/runner.log \
  systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide lmr sweep" $J/runner.sh
