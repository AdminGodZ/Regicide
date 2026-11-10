# Shared helpers for the truth-data jobs (source me).
J=~/regicide-jobs/truth; R4=~/regicide-jobs/r4; LOCKS=~/regicide-jobs/locks
mkdir -p $J $LOCKS
st() { echo "TR $* $(date -Is)" | tee -a $R4/status >> $J/status; }
training() { pgrep -f "[t]ools/nnue/train.py" > /dev/null; }
# another agent waiting for the timing lock (a flock process whose parent is not one of my truth jobs)
others_waiting() {
  local p pp
  for p in $(pgrep -x flock); do
    pp=$(ps -o ppid= -p $p | tr -d " ")
    tr "\0" " " < /proc/$pp/cmdline 2>/dev/null | grep -q "jobs/truth/" || return 0
  done
  return 1
}
# turn_wait: after one of my runs, let one run of another agent go first: wait while others are waiting until the
# lock is seen held by someone else (their turn started), or 90 s pass with the lock free.
# lock_holder_other: the timing lock is held by a process that is not one of my truth jobs
lock_holder_other() {  # (lslocks does not list these flock locks on this WSL kernel: probe instead)
  flock -n $LOCKS/timing.lock true && return 1      # free
  pgrep -f "[m]atch-ff0a3d2" > /dev/null && return 1 # one of my matches holds it
  return 0
}
turn_wait() {
  [ -e $J/my_last_release ] || return 0
  local t0=$(cat $J/my_last_release)
  while [ $(( $(date +%s) - t0 )) -lt 5 ]; do sleep 1; done  # my releasing subshell has exited
  while others_waiting; do
    lock_holder_other && return 0
    [ $(( $(date +%s) - t0 )) -gt 90 ] && return 0
    sleep 3
  done
}
# with_timing CMD...: run CMD holding the machine-wide timing lock and matchlock (background generators stopped).
# My self-play matches never overlap my own training runs: wait for none, take the lock, and if a training run
# started meanwhile, give the lock back and wait again.
with_timing() {
  (
    while :; do
      while training; do sleep 20; done
      turn_wait
      exec 9>$LOCKS/timing.lock
      flock 9
      training || break
      flock -u 9; exec 9>&-
    done
    touch $R4/matchlock; sleep 8
    st "timing lock + matchlock taken; bg: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-|[r]escore-|[l]ossprobe'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
    "$@"; rc=$?
    rm -f $R4/matchlock
    date +%s > $J/my_last_release
    st "matchlock removed, timing lock released (rc=$rc)"
    exit $rc
  )
}
# selfplay_match NAME_A NET_A OUTDIR SO MOVETIME [NAME_B NET_B [OPENINGS]]: A = SO with evalfile NET_A, B = SO built-in
# (or NET_B)
selfplay_match() {
  local na=$1 neta=$2 out=$3 so=$4 mt=$5 nb=${6:-n6c} netb=${7:-} op=${8:-$HOME/regicide-gen2-work/openings4.txt}
  local W6=~/regicide-nnue6-work
  local optb=(); [ -n "$netb" ] && optb=(--option-b "set evalfile $netb")
  # nets of mine (~/regicide-truth-work) must have passed the export check (train.sh writes NAME.ok)
  local x; for x in $neta $netb; do
    case $x in */regicide-truth-work/*.nnue) [ -s "${x%.nnue}.ok" ] || { st "match refused: $x has no passed export check"; return 1; } ;; esac
  done
  grep -q '^Elo (pent' $out/summary.txt 2>/dev/null && return 0
  rm -rf $out
  $W6/bin/match-ff0a3d2 --cbhost $W6/bin/cbhost -a $so -b $so --name-a $na --name-b $nb \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $neta" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" "${optb[@]}" \
    --openings $op --movetime $mt -c 10 --pdn-comments -o $out > $out.log 2>&1
}
