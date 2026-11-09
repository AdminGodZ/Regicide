#!/usr/bin/env bash
# Keeps background generators stopped (SIGSTOP on the process AND its bash parent: a non-job-control bash parent
# sends SIGCONT to a stopped foreground child) while their condition holds; resumes them afterwards.
#   gen4b   datagen-eb1037c-n4c : matchlock | egdblock | gen4bhold
#   gen5    datagen-635d949     : matchlock | egdblock | (gen5yield and a bookgen process is running unpaused)
#   bookgen bookgen-*           : matchlock | baselock
# With gen5yield set, exactly one ~10-12 thread generator runs in the background: bookgen while it exists, then gen5.
# Locks live in ~/regicide-jobs/r4. Runs until ~/regicide-jobs/r4/hold goes away.
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
st "pausekeeper5 start pid $$"
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
    if [ -z "${PAUSED[$g]:-}" ] && [ -n "$ps" ]; then sleep 1; st "pausekeeper5 stopped $g: $(states $ps)"; fi
    [ -n "$ps" ] && PAUSED[$g]="$ps"
  elif [ -n "${PAUSED[$g]:-}" ]; then
    local ps=$(fam "$pat")
    kill -CONT $ps ${PAUSED[$g]} 2>/dev/null
    st "pausekeeper5 resumed $g: ${PAUSED[$g]}"
    PAUSED[$g]=""
  fi
}
on() { [ -e $J/$1 ] && echo 1 || echo 0; }
while [ -e $J/hold ]; do
  M=$(on matchlock); E=$(on egdblock); H=$(on gen4bhold); B=$(on baselock); Y=$(on gen5yield)
  BK=$(( ! (M || B) )); [ -z "$(pgrep -f '[b]ookgen-')" ] && BK=0   # bookgen present and allowed to run
  handle gen4b '[d]atagen-eb1037c-n4c' $(( M || E || H ))
  handle gen5 '[d]atagen-635d949' $(( M || E || (Y && BK) ))
  handle bookgen '[b]ookgen-' $(( M || B ))
  sleep 3
done
handle gen4b '[d]atagen-eb1037c-n4c' 0; handle gen5 '[d]atagen-635d949' 0; handle bookgen '[b]ookgen-' 0
st "pausekeeper5 end"
