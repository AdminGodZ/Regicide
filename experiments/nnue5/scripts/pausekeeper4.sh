#!/usr/bin/env bash
# Keeps background generators stopped (SIGSTOP on the process AND its bash parent: a non-job-control bash parent
# sends SIGCONT to a stopped foreground child) while their condition holds; resumes them afterwards.
#   gen4b   datagen-eb1037c-n4c : matchlock | egdblock | gen4bhold
#   gen5    datagen-635d949     : matchlock | egdblock
#   bookgen bookgen-*           : matchlock | baselock   (book runs alongside the EGDB build)
# Locks live in ~/regicide-jobs/r4. A CheckerBoard baseline takes baselock (datagen keeps running alongside, as in
# the earlier baselines); A/B matches take matchlock. Runs until ~/regicide-jobs/r4/hold goes away.
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
st "pausekeeper4 start pid $$"
declare -A PAUSED
fam() {
  local out=""
  for p in $(pgrep -f "$1"); do
    local pp=$(ps -o ppid= -p $p | tr -d ' ')
    [ "$(ps -o comm= -p $pp 2>/dev/null)" = bash ] && out="$out $pp"
    out="$out $p"
  done
  echo $out
}
states() { local s=""; for p in $*; do s="$s $p:$(awk '/^State/{print $2}' /proc/$p/status 2>/dev/null)"; done; echo $s; }
handle() {  # handle GROUP PATTERN WANT
  local g=$1 pat=$2 want=$3
  if [ $want = 1 ]; then
    local ps=$(fam "$pat")
    [ -n "$ps" ] && kill -STOP $ps 2>/dev/null
    if [ -z "${PAUSED[$g]:-}" ] && [ -n "$ps" ]; then sleep 1; st "pausekeeper4 stopped $g: $(states $ps)"; fi
    [ -n "$ps" ] && PAUSED[$g]="$ps"
  elif [ -n "${PAUSED[$g]:-}" ]; then
    local ps=$(fam "$pat")
    kill -CONT $ps ${PAUSED[$g]} 2>/dev/null
    st "pausekeeper4 resumed $g: ${PAUSED[$g]}"
    PAUSED[$g]=""
  fi
}
on() { [ -e $J/$1 ] && echo 1 || echo 0; }
while [ -e $J/hold ]; do
  M=$(on matchlock); E=$(on egdblock); H=$(on gen4bhold); B=$(on baselock)
  handle gen4b '[d]atagen-eb1037c-n4c' $(( M || E || H ))
  handle gen5 '[d]atagen-635d949' $(( M || E ))
  handle bookgen '[b]ookgen-' $(( M || B ))
  sleep 3
done
handle gen4b '[d]atagen-eb1037c-n4c' 0; handle gen5 '[d]atagen-635d949' 0; handle bookgen '[b]ookgen-' 0
st "pausekeeper4 end"
