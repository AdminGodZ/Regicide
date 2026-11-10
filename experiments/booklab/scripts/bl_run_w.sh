#!/usr/bin/env bash
# booklab: one CheckerBoard match with the baseline settings (0.5 s/move by default, -c 8, hash 128, opponents with
# their own books and the KingsRow 2-8 WLD db, Regicide with its DLL's egdb/ folder, PDN comments on).
# bl_run.sh and bl_run_x.sh call this script (EXTRA_A / EXTRA_ARGS / MATCH unset there = their old behaviour).
#   bl_run_w.sh A B TAG DECK [MOVETIME] [CONC]
#   A, B: regicide:<dlldir> | cake | cake2 | kingsrow | kingsrow2 | guinn | guinn2
#   DECK: acf156 | path to an openings file
#   MATCH: runner (default the final-validation runner match-747ab51); EXTRA_ARGS: more runner arguments
#   EXTRA_A: one more engine command for A (e.g. set evalfile with the n8q net), followed by "get evalfile"
# Book mode (lead, 2026-11-10, review A3): every engine gets an explicit "set book 3" (CheckerBoard: best moves only)
# as its last option. Neither runner reads replies back, so before the match a pre-flight check starts every engine
# through the same cbhost with exactly the same option list and requires "get book" to answer 3; otherwise the run
# is refused (exit 3) and nothing is played. (Until 2026-11-10 no script sent set book: KingsRow played in its
# default mode 1, the others in 3.)
# Takes the machine-wide timing lock (~/regicide-jobs/locks/timing.lock) for the whole match (not the pre-flight).
# A run counts as finished only if the runner exited 0 (recorded in OUT/exit_status) and the summary says
# "Games: N of N scheduled"; otherwise it is replayed. The script exits with the runner's exit status.
# Must run inside an attached wsl.exe session (Windows interop). Output ~/regicide-match/results/bl-TAG.
set -u
A=$1; B=$2; TAG=$3; DECK=$4; MT=${5:-0.5}; CONC=${6:-8}
MATCH=${MATCH:-~/regicide-match/bin/match-747ab51}
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
PROBE=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/book_probe.py
L=~/regicide-jobs/booklab; OUT=~/regicide-match/results/bl-$TAG
LOCK=~/regicide-jobs/locks/timing.lock
st() { echo "$* $(date -Is)" >> $L/status; }
spec() {  # spec SIDE(a|b) ENGINE -> fills SPEC, DLL and OPTS
  local s=$1 e=$2
  OPTS=()
  case $e in
    regicide:*) DLL="$E\\regicide\\${e#regicide:}\\regicide_cb.dll"
                OPTS=(--option-$s "set hashsize 128" --option-$s "set threads 1") ;;
    cake)      DLL="$E\\cake\\cake_189f.dll" ;;
    cake2)     DLL="$E\\booklab\\cake2\\cake_189f.dll" ;;
    kingsrow)  DLL="$E\\kingsrow\\Kingsrow64.dll" ;;
    kingsrow2) DLL="$E\\booklab\\kingsrow2\\Kingsrow64.dll" ;;
    guinn)     DLL="$E\\guinn\\gui_nn64.dll" ;;
    guinn2)    DLL="$E\\booklab\\guinn2\\gui_nn64.dll" ;;
    *) echo "unknown engine $e"; exit 2 ;;
  esac
  SPEC="exec:$CB \"$DLL\""
  case $e in
    regicide:*) ;;
    *) OPTS=(--option-$s "set dbpath $DB8" --option-$s "set enable_wld 1" --option-$s "set dbmbytes 512"
             --option-$s "set max_dbpieces 8" --option-$s "set hashsize 128") ;;
  esac
  case $e in kingsrow*) OPTS+=(--option-$s "set searchthreads 1") ;; esac
}
spec a "$A"; SA=$SPEC; DA=$DLL; OA=("${OPTS[@]}")
[ -n "${EXTRA_A:-}" ] && OA+=(--option-a "$EXTRA_A" --option-a "get evalfile")
OA+=(--option-a "set book 3")
spec b "$B"; SB=$SPEC; DB=$DLL; OB=("${OPTS[@]}")
OB+=(--option-b "set book 3")
NAMES=()
[ "${A%2}" = "${B%2}" ] && NAMES=(--name-a "${A%2}-A" --name-b "${B%2}-B")
done_run() {
  [ -f "$OUT/summary.txt" ] && [ "$(cat "$OUT/exit_status" 2>/dev/null)" = "0" ] &&
    grep -Eq "^Games: ([0-9]+) of \1 scheduled" "$OUT/summary.txt" && grep -q "^A:" "$OUT/summary.txt"
}
if done_run; then st "BL skip $TAG"; exit 0; fi
# pre-flight: the same option list (minus the runner flags) through cbhost, then get book must answer 3
preflight() {  # preflight NAME DLL --option-x TEXT ...
  local name=$1 dll=$2; shift 2
  local args=() i=0
  for x in "$@"; do i=$((i + 1)); [ $((i % 2)) -eq 0 ] && [ "$x" != "set book 3" ] && args+=(--opt "$x"); done
  local r; r=$(python3 $PROBE --cbhost $CB --dll "$dll" "${args[@]}" --book 3 --readback-only 2>&1); local rc=$?
  echo "$name: $r"
  return $rc
}
PF=$(preflight A "$DA" "${OA[@]}" && preflight B "$DB" "${OB[@]}"); prc=$?
if [ $prc -ne 0 ]; then st "BL refused $TAG: book read-back differs: $(echo $PF)"; exit 3; fi
st "BL wait-lock $TAG $A vs $B deck $DECK mt $MT c $CONC"
exec 9>$LOCK
flock 9
if done_run; then flock -u 9; st "BL skip $TAG (finished while waiting)"; exit 0; fi
rm -rf "$OUT"
st "BL start $TAG $A vs $B; $(echo $PF); bg: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-|[l]ossprobe'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
"$MATCH" ${EXTRA_ARGS:-} -a "$SA" -b "$SB" "${NAMES[@]}" "${OA[@]}" "${OB[@]}" \
  --openings $DECK --movetime $MT -c $CONC --pdn-comments -o "$OUT" > "$L/bl-$TAG.log" 2>&1
rc=$?
flock -u 9
mkdir -p "$OUT"; echo $rc > "$OUT/exit_status"
st "BL end $TAG rc=$rc $(grep '^Games:' $OUT/summary.txt 2>/dev/null) $(grep '^A:' $OUT/summary.txt 2>/dev/null) $(grep '^Elo (pent' $OUT/summary.txt 2>/dev/null)"
done_run || { [ $rc -eq 0 ] && rc=4; st "BL INCOMPLETE $TAG (exit $rc)"; }
exit $rc
