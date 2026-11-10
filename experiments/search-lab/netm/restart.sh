#!/bin/bash
# Waits for the first net runner to exit (STOP set), marks its finished chunks, starts runner2 in the same unit.
N=~/regicide-jobs/netm
while systemctl --user is-active -q regicide-netm; do sleep 30; done
for d in $N/matches/M*/; do
  for c in a b c d e f; do
    [ -e $d/$c.rc ] && continue
    /usr/bin/grep -qE '^Games: ([0-9]+) of \1 scheduled$' $d/$c/summary.txt 2>/dev/null && echo unrecorded > $d/$c.rc
  done
done
echo "RESTART: first runner gone, runner2 starts $(date -Is)" >> $N/runner.log
rm -f $N/STOP
exec systemd-run --user --unit=regicide-netm --collect -p Nice=10 -p WorkingDirectory=$N \
  -p StandardOutput=append:$N/runner.log -p StandardError=append:$N/runner.log \
  systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide net matches" $N/runner2.sh
