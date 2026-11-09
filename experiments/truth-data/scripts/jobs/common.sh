# Shared helpers for the truth-data jobs (source me).
J=~/regicide-jobs/truth; R4=~/regicide-jobs/r4; LOCKS=~/regicide-jobs/locks
mkdir -p $J $LOCKS
st() { echo "TR $* $(date -Is)" | tee -a $R4/status >> $J/status; }
# with_timing CMD...: run CMD holding the machine-wide timing lock and matchlock (background generators stopped).
with_timing() {
  # my self-play matches never overlap my own training runs
  while pgrep -f "[t]ools/nnue/train.py" > /dev/null; do sleep 20; done
  (
    exec 9>$LOCKS/timing.lock
    flock 9
    touch $R4/matchlock; sleep 8
    st "timing lock + matchlock taken; bg: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-|[r]escore-|[l]ossprobe'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
    "$@"; rc=$?
    rm -f $R4/matchlock
    st "matchlock removed, timing lock released (rc=$rc)"
    exit $rc
  )
}
# selfplay_match NAME_A NET_A OUTDIR SO MOVETIME [NAME_B NET_B]: A = SO with evalfile NET_A, B = SO built-in (or NET_B)
selfplay_match() {
  local na=$1 neta=$2 out=$3 so=$4 mt=$5 nb=${6:-n6c} netb=${7:-}
  local W6=~/regicide-nnue6-work
  local optb=(); [ -n "$netb" ] && optb=(--option-b "set evalfile $netb")
  grep -q '^Elo (pent' $out/summary.txt 2>/dev/null && return 0
  rm -rf $out
  $W6/bin/match-ff0a3d2 --cbhost $W6/bin/cbhost -a $so -b $so --name-a $na --name-b $nb \
    --option-a "set threads 1" --option-a "set hashsize 128" --option-a "set book 0" --option-a "set evalfile $neta" \
    --option-b "set threads 1" --option-b "set hashsize 128" --option-b "set book 0" "${optb[@]}" \
    --openings ~/regicide-gen2-work/openings4.txt --movetime $mt -c 10 --pdn-comments -o $out > $out.log 2>&1
}
