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
# A run counts as finished only if the runner exited 0 (recorded in OUT/exit_status) and it played all its games
# ("Games: N of N scheduled"; for a fixed-colour deck N = its number of lines); otherwise it is replayed. The script exits with the runner's exit status.
# Memory (2026-11-10 20:16: 16 engine crashes in k3-kingsrow-a-h1 at the start of the run, Windows commit exhausted
# by the WSL VM's page cache): after taking the lock, Windows free commit must be at least MINCOMMIT GB (default 14);
# below that the WSL page cache is dropped (as root through wsl.exe; only clean cached file pages are discarded) and
# the check repeats every 30 s for up to 10 minutes; the values are written to the start line.
# A run with any crash, time-out, illegal-move, protocol-error or no-new-game termination is not finished either
# (status line "BL CRASHES", exit 5).
# Must run inside an attached wsl.exe session (Windows interop). Output ~/regicide-match/results/bl-TAG.
set -u
A=$1; B=$2; TAG=$3; DECK=$4; MT=${5:-0.5}; CONC=${6:-8}
MATCH=${MATCH:-~/regicide-match/bin/match-747ab51}
CB=${CBHOST:-/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe}  # CBHOST: e.g. the PR #49 host (T1, WP12)
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
PROBE=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/book_probe.py
L=~/regicide-jobs/booklab; OUT=~/regicide-match/results/bl-$TAG
LOCK=~/regicide-jobs/locks/timing.lock
# NOLOCK=1 (lead 2026-11-11, match-order run at concurrency 1 "in the background of everything else"): a private lock
# file instead of the machine-wide timing lock; the start line says so.
[ -n "${NOLOCK:-}" ] && LOCK=$L/nolock-$TAG.lock
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
# SHIMDIR: every engine runs behind tools/booklab/cbshim.py, which logs each protocol line with a timestamp
# (SHIMDIR/A, SHIMDIR/B; T1's per-move log of wall time, host time and offered maxtime)
SHIM=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/cbshim.py
shimmed() { [ -n "${SHIMDIR:-}" ] && echo "exec:/usr/bin/python3 $SHIM $SHIMDIR/$1 $CB \"$2\"" || echo "$3"; }
spec a "$A"; SA=$(shimmed A "$DLL" "$SPEC"); DA=$DLL; OA=("${OPTS[@]}")
[ -n "${EXTRA_A:-}" ] && OA+=(--option-a "$EXTRA_A" --option-a "get evalfile")
OA+=(--option-a "set book 3")
spec b "$B"; SB=$(shimmed B "$DLL" "$SPEC"); DB=$DLL; OB=("${OPTS[@]}")
OB+=(--option-b "set book 3")
NAMES=()
[ "${A%2}" = "${B%2}" ] && NAMES=(--name-a "${A%2}-A" --name-b "${B%2}-B")
bad_games() { grep -h "^\[Termination" "$OUT/games.pdn" 2>/dev/null | grep -c -E "crashed|did not start|timed out|illegal|protocol error"; }
# Games a finished run must have played (review 2026-11-11): one per deck row with a fixed colour for A
# ("| A=B" / "| A=W"), two per ordinary row (both colours), times --rounds N in EXTRA_ARGS. The runner's summary
# says "of 2N scheduled" for fixed-colour rows (2026-11-10, w13-ev-cake: 40 of 80), so it is not used for decks with
# such rows. With --pairs in EXTRA_ARGS the count is not derived (the run is then checked against the summary only
# for decks without fixed-colour rows; with both, nothing counts as finished and the wrapper says so).
expected_games() {
  local rows fixed rounds
  rows=$(grep -v '^[[:space:]]*#' "$DECK" 2>/dev/null | grep -c '[^[:space:]]')
  fixed=$(grep -v '^[[:space:]]*#' "$DECK" 2>/dev/null | grep -Ec '\|[[:space:]]*A=[BW][[:space:]]*$')
  rounds=$(echo " ${EXTRA_ARGS:-} " | sed -n 's/.* --rounds \([0-9][0-9]*\) .*/\1/p'); rounds=${rounds:-1}
  if [ ! -f "$DECK" ] || [ "${fixed:-0}" = 0 ]; then
    sed -n 's/^Games: [0-9]* of \([0-9]*\) scheduled.*/\1/p' "$OUT/summary.txt" 2>/dev/null
  elif echo " ${EXTRA_ARGS:-} " | grep -q -- " --pairs "; then
    echo ""
  else
    echo $(( (fixed + 2 * (rows - fixed)) * rounds ))
  fi
}
done_run() {
  [ -f "$OUT/summary.txt" ] && [ "$(cat "$OUT/exit_status" 2>/dev/null)" = "0" ] &&
    [ -n "$(expected_games)" ] && grep -Eq "^Games: $(expected_games) of [0-9]+ scheduled" "$OUT/summary.txt" &&
    grep -q "^A:" "$OUT/summary.txt" && [ "$(bad_games)" = "0" ]
}
commit_free() { /mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -NoProfile -Command \
  '[math]::Round((Get-CimInstance Win32_OperatingSystem).FreeVirtualMemory/1MB,1)' 2>/dev/null | tr -d '\r'; }
ensure_memory() {  # sets MEM (text for the start line)
  local min=${MINCOMMIT:-14} c t=0
  c=$(commit_free); MEM="commit free ${c} GB"
  while awk -v c="$c" -v m="$min" 'BEGIN { exit !(c < m) }'; do
    /mnt/c/Windows/System32/wsl.exe -u root -- sh -c 'sync; echo 1 > /proc/sys/vm/drop_caches' > /dev/null 2>&1
    sleep 30; t=$((t + 30)); c=$(commit_free); MEM="$MEM; page cache dropped, ${c} GB after ${t} s"
    [ $t -ge 600 ] && { MEM="$MEM; STILL LOW, started anyway"; break; }
  done
}
if done_run; then st "BL skip $TAG"; exit 0; fi
# Hold: a tag matching a line of $L/HOLD_TAGS (extended regular expressions) is not played (exit 6), so that a
# chain stops at that run instead of being killed while a match plays.
if [ -s $L/HOLD_TAGS ] && echo "$TAG" | grep -Eqf $L/HOLD_TAGS; then st "BL held $TAG (HOLD_TAGS)"; exit 6; fi
# pre-flight: the same option list (minus the runner flags) through cbhost, then get book must answer 3
preflight() {  # preflight NAME DLL --option-x TEXT ...
  local name=$1 dll=$2; shift 2
  local args=() i=0
  for x in "$@"; do i=$((i + 1)); [ $((i % 2)) -eq 0 ] && [ "$x" != "set book 3" ] && args+=(--opt "$x"); done
  local r; r=$(python3 $PROBE --cbhost $CB --dll "$dll" "${args[@]}" --book 3 --readback-only 2>&1); local rc=$?
  echo "$name: $r"
  return $rc
}
st "BL wait-lock $TAG $A vs $B deck $DECK mt $MT c $CONC"
exec 9>$LOCK
flock 9
if done_run; then flock -u 9; st "BL skip $TAG (finished while waiting)"; exit 0; fi
# The pre-flight runs under the lock (review 2026-11-10 a): loading Regicide initialises its database.
PF=$(preflight A "$DA" "${OA[@]}" && preflight B "$DB" "${OB[@]}"); prc=$?
if [ $prc -eq 0 ] && [[ "${EXTRA_A:-}" == "set param "* ]]; then
  want=${EXTRA_A#set param }
  got=$(python3 $PROBE --cbhost $CB --dll "$DA" --opt "$EXTRA_A" --book 3 --readback-only --show-replies 2>&1 | grep -F "set param" | head -1)
  PF="$PF; param: $got"
  [[ "$got" == *"reply 1 $want"* ]] || prc=1
fi
if [ $prc -ne 0 ]; then flock -u 9; st "BL refused $TAG: book read-back differs: $(echo $PF)"; exit 3; fi
rm -rf "$OUT"
[ -n "${SHIMDIR:-}" ] && { rm -rf "$SHIMDIR"; mkdir -p "$SHIMDIR"; }
ensure_memory
st "BL start $TAG $A vs $B; ${NOLOCK:+NO TIMING LOCK; }$(echo $PF); $MEM; bg: $(for p in $(pgrep -f '[d]atagen-|[b]ookgen-|[l]ossprobe'); do echo -n "$(ps -o comm= -p $p):$(awk '/^State/{print $2}' /proc/$p/status) "; done)"
"$MATCH" ${EXTRA_ARGS:-} -a "$SA" -b "$SB" "${NAMES[@]}" "${OA[@]}" "${OB[@]}" \
  --openings $DECK --movetime $MT -c $CONC --pdn-comments -o "$OUT" > "$L/bl-$TAG.log" 2>&1
rc=$?
# The marker is written and checked before the lock is released (review 2026-11-10 b), so a second caller waiting for
# the lock sees a finished run and skips it.
mkdir -p "$OUT"; echo $rc > "$OUT/exit_status"
st "BL end $TAG rc=$rc $(grep '^Games:' $OUT/summary.txt 2>/dev/null) $(grep '^A:' $OUT/summary.txt 2>/dev/null) $(grep '^Elo (pent' $OUT/summary.txt 2>/dev/null)"
nb=$(bad_games)
if [ "$nb" != "0" ]; then
  [ $rc -eq 0 ] && rc=5; st "BL CRASHES $TAG: $nb games ended by crash/time-out/illegal move/no new game (exit $rc)"
elif ! done_run; then
  [ $rc -eq 0 ] && rc=4; st "BL INCOMPLETE $TAG (exit $rc)"
fi
flock -u 9
exit $rc
