#!/usr/bin/env bash
# Companion of ~/regicide-jobs/r4/pausekeeper5.sh for jobs that are not in its hard-coded list.
# Group 1: every process whose NAME (comm, first 15 characters; pgrep without -f) matches a pattern in
#   ~/regicide-jobs/locks/bg_patterns (one regex per line, '#' comments; e.g. '^rescore-tr') is stopped (SIGSTOP on it
#   and its bash parent, as pausekeeper5 does) while ~/regicide-jobs/r4/matchlock exists.
# Group 2: names in ~/regicide-jobs/locks/bg_patterns_yield (the truth agent's threads beyond its base budget of 8)
#   are also stopped while ~/regicide-jobs/locks/yield_truth exists (the truth agent's own CheckerBoard runs) or another
#   agent computes in the background (a running, not stopped, process named bookgen-*, lossprobe* or blk-*), or anyone
#   holds ~/regicide-jobs/locks/timing.lock (a match is being played).
# Patterns are re-read every cycle. Names, not command lines: a command line can mention a pattern without being the job
# (2026-11-09: a -f version stopped an unrelated python process). Runs while ~/regicide-jobs/locks/pk_hold exists.
L=~/regicide-jobs/locks; R4=~/regicide-jobs/r4; mkdir -p $L; touch $L/pk_hold $L/bg_patterns $L/bg_patterns_yield
st() { echo "PKT $* $(date -Is)" >> $R4/status; }
st "pausekeeper_truth start pid $$"
fam() {  # fam PATTERN_FILE: matching pids and their bash parents, sorted, space separated
  local out="" p pp pat
  while read -r pat; do
    [ -z "$pat" ] || [ "${pat:0:1}" = "#" ] && continue
    for p in $(pgrep "$pat"); do
      pp=$(ps -o ppid= -p $p | tr -d ' ')
      [ "$(ps -o comm= -p $pp 2>/dev/null)" = bash ] && out="$out $pp"
      out="$out $p"
    done
  done < $1
  printf '%s\n' $out | sort -un | xargs
}
merge() { printf '%s\n' $* | sort -un | xargs; }
other_computing() {
  local p
  flock -n $L/timing.lock true || return 0   # someone plays (holds the timing lock): the yield group stops too
  for p in $(pgrep "^bookgen-|^lossprobe|^blk-"); do
    [ "$(awk '/^State/{print $2}' /proc/$p/status 2>/dev/null)" != T ] && return 0
  done
  return 1
}
PAUSED=""; YPAUSED=""
while [ -e $L/pk_hold ]; do
  if [ -e $R4/matchlock ] || [ -e $L/yield_truth ] || other_computing; then
    ys=$(fam $L/bg_patterns_yield)
    if [ -n "$ys" ]; then
      kill -STOP $ys 2>/dev/null
      new=$(merge $YPAUSED $ys)
      [ "$new" != "$YPAUSED" ] && st "yield-stopped: $ys"
      YPAUSED=$new
    fi
  elif [ -n "$YPAUSED" ]; then
    kill -CONT $YPAUSED 2>/dev/null; st "yield-resumed: $YPAUSED"; YPAUSED=""
  fi
  if [ -e $R4/matchlock ]; then
    ps_=$(fam $L/bg_patterns)
    if [ -n "$ps_" ]; then
      kill -STOP $ps_ 2>/dev/null
      new=$(merge $PAUSED $ps_)
      [ "$new" != "$PAUSED" ] && st "stopped: $ps_"
      PAUSED=$new
    fi
  elif [ -n "$PAUSED" ]; then
    kill -CONT $PAUSED 2>/dev/null
    st "resumed: $PAUSED"; PAUSED=""
  fi
  sleep 3
done
[ -n "$PAUSED$YPAUSED" ] && kill -CONT $PAUSED $YPAUSED 2>/dev/null
st "pausekeeper_truth end"
