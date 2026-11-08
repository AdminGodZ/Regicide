#!/usr/bin/env bash
# Pause the background generators (matchlock -> pausekeeper5), run speed_check.py, release.
J=~/regicide-jobs/r4
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 10; done
touch $J/matchlock; sleep 8
echo "EGDB8 speed check start $(date -Is)" >> $J/status
{ date -Is; uptime; for p in $(pgrep -f '[d]atagen-|[b]ookgen-'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done; echo
  powershell.exe -NoProfile -Command "Get-CimInstance Win32_OperatingSystem | % { 'Windows RAM total {0:N1} GB, free {1:N1} GB' -f (\$_.TotalVisibleMemorySize/1MB), (\$_.FreePhysicalMemory/1MB) }"
  python3 /mnt/c/regicide-match/jobs/egdb8/speed_check.py
  powershell.exe -NoProfile -Command "Get-CimInstance Win32_OperatingSystem | % { 'Windows RAM free after {0:N1} GB' -f (\$_.FreePhysicalMemory/1MB) }"
  date -Is; } > ~/regicide-jobs/egdb8/speed_check.txt 2>&1
rm -f $J/matchlock
echo "EGDB8 speed check done $(date -Is)" >> $J/status
