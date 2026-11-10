#!/usr/bin/env bash
# booklab: one CheckerBoard match with the baseline settings (0.5 s/move by default, -c 8, hash 128, opponents with
# their own books and the KingsRow 2-8 WLD db, Regicide with its DLL's egdb/ folder, PDN comments on).
#   bl_run.sh A B TAG DECK [MOVETIME] [CONC]
#   A, B: regicide:<dlldir> | cake | cake2 | kingsrow | kingsrow2 | guinn | guinn2
#   DECK: acf156 | path to an openings file
# Takes the machine-wide timing lock (~/regicide-jobs/locks/timing.lock) for the whole match.
# Must run inside an attached wsl.exe session (Windows interop). Output ~/regicide-match/results/bl-TAG.
set -u
A=$1; B=$2; TAG=$3; DECK=$4; MT=${5:-0.5}; CONC=${6:-8}
MATCH=${MATCH:-~/regicide-match/bin/match-747ab51}  # WP9 sets MATCH to the booklab build (A=B/A=W openings)
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
L=~/regicide-jobs/booklab; OUT=~/regicide-match/results/bl-$TAG
LOCK=~/regicide-jobs/locks/timing.lock
st() { echo "$* $(date -Is)" >> $L/status; }
spec() {  # spec SIDE(a|b) ENGINE -> fills SPEC and OPTS
  local s=$1 e=$2
  OPTS=()
  case $e in
    regicide:*) SPEC="exec:$CB \"$E\\regicide\\${e#regicide:}\\regicide_cb.dll\""
                OPTS=(--option-$s "set hashsize 128" --option-$s "set threads 1") ;;
    cake)      SPEC="exec:$CB \"$E\\cake\\cake_189f.dll\"" ;;
    cake2)     SPEC="exec:$CB \"$E\\booklab\\cake2\\cake_189f.dll\"" ;;
    kingsrow)  SPEC="exec:$CB \"$E\\kingsrow\\Kingsrow64.dll\"" ;;
    kingsrow2) SPEC="exec:$CB \"$E\\booklab\\kingsrow2\\Kingsrow64.dll\"" ;;
    guinn)     SPEC="exec:$CB \"$E\\guinn\\gui_nn64.dll\"" ;;
    guinn2)    SPEC="exec:$CB \"$E\\booklab\\guinn2\\gui_nn64.dll\"" ;;
    *) echo "unknown engine $e"; exit 2 ;;
  esac
  case $e in
    regicide:*) ;;
    *) OPTS=(--option-$s "set dbpath $DB8" --option-$s "set enable_wld 1" --option-$s "set dbmbytes 512"
             --option-$s "set max_dbpieces 8" --option-$s "set hashsize 128") ;;
  esac
  case $e in kingsrow*) OPTS+=(--option-$s "set searchthreads 1") ;; esac
}
spec a "$A"; SA=$SPEC; OA=("${OPTS[@]}")
spec b "$B"; SB=$SPEC; OB=("${OPTS[@]}")
NAMES=()
[ "${A%2}" = "${B%2}" ] && NAMES=(--name-a "${A%2}-A" --name-b "${B%2}-B")
# A run is done when its summary has the A: line and was not interrupted or stopped (fixed-colour WP9 decks print no
# pentanomial Elo, so that line is not the test). Checked before and again after taking the lock, so a second
# waiter for a run that is in progress skips it instead of deleting and replaying it.
done_run() { [ -f "$OUT/summary.txt" ] && grep -q "^A:" "$OUT/summary.txt" && ! grep -q "interrupted\|stopped:" "$OUT/summary.txt"; }
if done_run; then st "BL skip $TAG"; exit 0; fi
st "BL wait-lock $TAG $A vs $B deck $DECK mt $MT c $CONC"
exec 9>$LOCK
flock 9
if done_run; then flock -u 9; st "BL skip $TAG (finished while waiting)"; exit 0; fi
rm -rf "$OUT"
st "BL start $TAG $A vs $B; bg: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-|[l]ossprobe'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
"$MATCH" -a "$SA" -b "$SB" "${NAMES[@]}" "${OA[@]}" "${OB[@]}" \
  --openings $DECK --movetime $MT -c $CONC --pdn-comments -o "$OUT" > "$L/bl-$TAG.log" 2>&1
rc=$?
flock -u 9
st "BL end $TAG rc=$rc $(grep '^A:' $OUT/summary.txt) $(grep '^Elo (pent' $OUT/summary.txt)"
