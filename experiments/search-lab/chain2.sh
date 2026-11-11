#!/bin/bash
# After M6a: n9h vs n8q (M7) has priority over the LMR sweep whenever n9h exists on origin/t3/truth-data; the sweep
# runs otherwise and is paused at a test boundary (STOP) when n9h appears, then resumed after M7.
N=~/regicide-jobs/netm; J=~/regicide-jobs/searchlab; REPO=~/Documents/Projects/Regicide
log() { echo "CHAIN2 $* $(date -Is)" >> $J/runner.log; }
m7done() { /usr/bin/grep -q '^M7_' $N/done.txt; }
n9h() { [ -s $N/nets/n9h.nnue ] && return 0; git -C $REPO fetch -q origin t3/truth-data 2>/dev/null
        git -C $REPO cat-file -e origin/t3/truth-data:experiments/truth-data/n9h.nnue 2>/dev/null; }
active() { systemctl --user is-active -q $1; }
start_net() { rm -f $N/STOP; systemd-run --user --unit=regicide-netm --collect -p Nice=10 -p WorkingDirectory=$N \
  -p StandardOutput=append:$N/runner.log -p StandardError=append:$N/runner.log \
  systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide net matches" $N/runner2.sh; }
start_sweep() { rm -f $J/STOP; systemd-run --user --unit=regicide-searchlab --collect -p Nice=10 -p WorkingDirectory=$J \
  -p StandardOutput=append:$J/runner.log -p StandardError=append:$J/runner.log \
  systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide lmr sweep" $J/runner.sh; }
sweep_left() { /usr/bin/grep -avxFf $J/done.txt $J/queue.txt | /usr/bin/grep -av '^#' | /usr/bin/grep -ac .; }
until /usr/bin/grep -q '^M6a_' $N/done.txt; do sleep 60; done
log "M6a done"
while :; do
  if ! m7done && n9h; then
    if active regicide-searchlab; then log "n9h available: pausing sweep"; touch $J/STOP
      while active regicide-searchlab; do sleep 30; done; fi
    active regicide-netm || { log "starting net runner for M7"; start_net; }
    until m7done; do sleep 60; done
    log "M7 done"
  fi
  [ "$(sweep_left)" = 0 ] && ! active regicide-searchlab && { log "sweep queue empty"; break; }
  if active regicide-netm; then touch $N/STOP; systemctl --user stop regicide-netm; log "net runner stopped (idle)"; fi
  active regicide-searchlab || { log "starting sweep runner"; start_sweep; }
  sleep 300
done
