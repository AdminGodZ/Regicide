#!/usr/bin/env bash
# While ~/regicide-jobs/r4/matchlock exists, keep every datagen process AND its bash parent stopped (a
# non-job-control bash parent sends SIGCONT to a stopped child, which undid plain `kill -STOP datagen`).
# Logs the observed process states; resumes both when the lock goes away. Runs until ~/regicide-jobs/r4/hold goes.
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
st "pausekeeper start pid $$"
while [ -e $J/hold ]; do
  while [ ! -e $J/matchlock ] && [ -e $J/hold ]; do sleep 1; done
  [ -e $J/hold ] || break
  PIDS=""; logged=0
  while [ -e $J/matchlock ]; do
    for p in $(pgrep -f '[d]atagen-'); do
      pp=$(ps -o ppid= -p $p | tr -d ' ')
      case "$(ps -o comm= -p $pp 2>/dev/null)" in bash) kill -STOP $pp 2>/dev/null; PIDS="$PIDS $pp";; esac
      kill -STOP $p 2>/dev/null; PIDS="$PIDS $p"
    done
    PIDS=$(echo $PIDS | tr ' ' '\n' | sort -u | tr '\n' ' ')
    if [ $logged -eq 0 ] && [ -n "$PIDS" ]; then
      sleep 2; S=""; for p in $PIDS; do S="$S $p:$(awk '/^State/{print $2}' /proc/$p/status)"; done
      st "pausekeeper stopped$S"; logged=1
    fi
    sleep 5
  done
  for p in $(pgrep -f '[d]atagen-'); do kill -CONT $p 2>/dev/null; done
  for p in $PIDS; do kill -CONT $p 2>/dev/null; done
  [ -n "$PIDS" ] && st "pausekeeper resumed $PIDS"
done
st "pausekeeper end"
