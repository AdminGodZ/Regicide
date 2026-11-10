for p in $(pgrep -x flock); do pp=$(ps -o ppid= -p $p | tr -d ' '); echo "flock $p ($(ps -o etime= -p $p)) parent: $(ps -o args= -p $pp | cut -c1-150)"; done
tail -4 ~/regicide-jobs/booklab/status | cut -c1-200
tail -3 ~/regicide-jobs/truth/status | cut -c1-200
ls -la ~/regicide-jobs/r4/matchlock 2>/dev/null
cat /proc/loadavg
