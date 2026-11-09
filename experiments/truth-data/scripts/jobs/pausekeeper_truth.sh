#!/usr/bin/env bash
# Companion of ~/regicide-jobs/r4/pausekeeper5.sh for jobs that are not in its hard-coded list.
# Every process whose NAME (comm, first 15 characters; pgrep without -f) matches a pattern in
# ~/regicide-jobs/locks/bg_patterns (one extended regex per line, '#' comments; e.g. '^rescore-tr') is stopped
# (SIGSTOP on it and its bash parent, as pausekeeper5 does) while ~/regicide-jobs/r4/matchlock exists, and resumed
# when it goes away. Patterns are re-read every cycle, so a job is registered by appending its pattern.
# Names, not command lines: a command line can mention a pattern without being the job (2026-11-09: a -f version
# stopped an unrelated python process). Runs while ~/regicide-jobs/locks/pk_hold exists.
L=~/regicide-jobs/locks; R4=~/regicide-jobs/r4; mkdir -p $L; touch $L/pk_hold $L/bg_patterns
st() { echo "PKT $* $(date -Is)" >> $R4/status; }
st "pausekeeper_truth start pid $$"
PAUSED=""
fam() {
  local out=""
  while read -r pat; do
    [ -z "$pat" ] || [ "${pat:0:1}" = "#" ] && continue
    for p in $(pgrep "$pat"); do
      local pp=$(ps -o ppid= -p $p | tr -d ' ')
      [ "$(ps -o comm= -p $pp 2>/dev/null)" = bash ] && out="$out $pp"
      out="$out $p"
    done
  done < $L/bg_patterns
  echo $out | tr ' ' '\n' | sort -u | tr '\n' ' '
}
while [ -e $L/pk_hold ]; do
  if [ -e $R4/matchlock ]; then
    ps_=$(fam)
    if [ -n "$ps_" ]; then
      kill -STOP $ps_ 2>/dev/null
      [ "$ps_" != "$PAUSED" ] && st "stopped: $ps_"
      PAUSED=$(echo $PAUSED $ps_ | tr ' ' '\n' | sort -u | tr '\n' ' ')
    fi
  elif [ -n "$PAUSED" ]; then
    kill -CONT $PAUSED 2>/dev/null
    st "resumed: $PAUSED"; PAUSED=""
  fi
  sleep 3
done
[ -n "$PAUSED" ] && kill -CONT $PAUSED 2>/dev/null
st "pausekeeper_truth end"
