#!/usr/bin/env bash
# One ponder-test CheckerBoard run (baseline settings: acf156, 0.5 s/move by default, opponents with their books and
# the KingsRow 2-8 WLD db, Regicide with its DLL's egdb/ folder = 2-8 pieces).
#   run_p.sh TAG OPP CFG BOOK [PAIRS] [DLLDIR]
#   OPP: cake | kingsrow | guinn;  CFG: A (threads 1, ponder 0, hash 128, -c 8) | B (threads 1, ponder 1, hash 128,
#   -c 8) | C (threads 4, ponder 1, hash 512, -c 4);  BOOK: 0 (set book 0) | 1 (DLL default, book on)
# Takes ~/regicide-jobs/locks/timing.lock for the whole match; B and C also create ~/regicide-jobs/r4/matchlock
# (after waiting for anyone else's to go away) and remove it right after the match.
# Must run inside an attached wsl.exe session (Windows interop). Output ~/regicide-match/results/pt-TAG.
set -u
TAG=$1; OPP=$2; CFG=$3; BOOK=$4; PAIRS=${5:-}; DLLDIR=${6:-ponder-df7780a}
MT=${MT:-0.5}
MATCH=~/regicide-match/bin/match-ponder-df7780a
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
L=~/regicide-jobs/ponder; R4=~/regicide-jobs/r4; LOCKS=~/regicide-jobs/locks
OUT=~/regicide-match/results/pt-$TAG
mkdir -p $L $LOCKS; touch $LOCKS/timing.lock
st() { echo "PT $* $(date -Is)" | tee -a $R4/status >> $L/status; }
REG="exec:$CB \"$E\\regicide\\$DLLDIR\\regicide_cb.dll\""
case $CFG in
  A) RX=(--option-a "set hashsize 128" --option-a "set threads 1" --option-a "set ponder 0"); CONC=8 ;;
  B) RX=(--option-a "set hashsize 128" --option-a "set threads 1" --option-a "set ponder 1"); CONC=8 ;;
  C) RX=(--option-a "set hashsize 512" --option-a "set threads 4" --option-a "set ponder 1"); CONC=4 ;;
  *) echo "bad CFG"; exit 2 ;;
esac
[ "$BOOK" = 0 ] && RX+=(--option-a "set book 0")
DBO=(--option-b "set dbpath $DB8" --option-b "set enable_wld 1" --option-b "set dbmbytes 512" --option-b "set max_dbpieces 8" --option-b "set hashsize 128")
case $OPP in
  kingsrow) DLL="$E\\kingsrow\\Kingsrow64.dll"; OX=(--option-b "set searchthreads 1") ;;
  cake)     DLL="$E\\cake\\cake_189f.dll"; OX=() ;;
  guinn)    DLL="$E\\guinn\\gui_nn64.dll"; OX=() ;;
  *) echo "bad OPP"; exit 2 ;;
esac
PX=(); [ -n "$PAIRS" ] && PX=(--pairs $PAIRS)
if [ -f "$OUT/summary.txt" ] && grep -q "^Elo (pent" "$OUT/summary.txt"; then st "skip $TAG"; exit 0; fi
bg() { for p in $(pgrep -f '[d]atagen-|[b]ookgen-|[r]escore-|[l]ossprobe|[t]ruth-tr|[t]rain.py'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done; }
st "wait-lock $TAG $OPP cfg $CFG book $BOOK c $CONC ${PAIRS:+pairs $PAIRS }mt $MT dll $DLLDIR"
exec 9>$LOCKS/timing.lock
flock 9
MINE=0
cleanup() { [ $MINE = 1 ] && rm -f $R4/matchlock && st "matchlock removed ($TAG)"; MINE=0; }
trap 'cleanup; exit 130' INT TERM
if [ $CFG != A ]; then
  while [ -e $R4/matchlock ]; do sleep 10; done   # someone else's: wait
  touch $R4/matchlock; MINE=1; sleep 8
fi
rm -rf "$OUT"
st "start $TAG $OPP cfg $CFG book $BOOK; matchlock $MINE; load $(cut -d' ' -f1 /proc/loadavg); bg: $(bg)"
"$MATCH" -a "$REG" -b "exec:$CB \"$DLL\"" --name-a "Regicide-$CFG" "${RX[@]}" "${DBO[@]}" "${OX[@]}" \
  --openings acf156 "${PX[@]}" --movetime $MT -c $CONC --pdn-comments -o "$OUT" > "$L/pt-$TAG.log" 2>&1
rc=$?
cleanup
flock -u 9
st "end $TAG rc=$rc $(grep '^A:' $OUT/summary.txt) | $(grep '^Elo (pent' $OUT/summary.txt) | $(grep 'ponder:' $OUT/summary.txt)"
