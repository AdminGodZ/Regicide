#!/usr/bin/env bash
# One CheckerBoard run in the ponder agent's config A (jobs/ponder/run_p.sh): DLL ponder-df7780a, threads 1, ponder 0,
# hash 128, -c 8, book off, acf156 x 2 colours, 0.5 s/move; opponents with their books and the KingsRow 2-8 WLD db;
# no matchlock (background jobs keep running, as in the ponder agent's n6c runs). Adds "set evalfile NET".
#   run_cb.sh TAG OPP NET(windows path or builtin)
# Must run inside an attached wsl.exe session (Windows interop). Output ~/regicide-match/results/tr-TAG.
source /mnt/c/regicide-match/jobs/truth/common.sh
set -u
TAG=$1; OPP=$2; NET=$3; MT=${MT:-0.5}
MATCH=~/regicide-match/bin/match-ponder-df7780a
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
OUT=~/regicide-match/results/tr-$TAG
REG="exec:$CB \"$E\regicide\ponder-df7780a\regicide_cb.dll\""
RX=(--option-a "set hashsize 128" --option-a "set threads 1" --option-a "set ponder 0" --option-a "set book 0" --option-a "set evalfile $NET")
DBO=(--option-b "set dbpath $DB8" --option-b "set enable_wld 1" --option-b "set dbmbytes 512" --option-b "set max_dbpieces 8" --option-b "set hashsize 128")
case $OPP in
  kingsrow) DLL="$E\kingsrow\Kingsrow64.dll"; OX=(--option-b "set searchthreads 1") ;;
  cake)     DLL="$E\cake\cake_189f.dll"; OX=() ;;
  guinn)    DLL="$E\guinn\gui_nn64.dll"; OX=() ;;
esac
if grep -q "^Elo (pent" "$OUT/summary.txt" 2>/dev/null; then st "cb skip $TAG"; exit 0; fi
st "cb wait-lock $TAG $OPP net $NET"
(
  turn_wait
  exec 9>$LOCKS/timing.lock; flock 9
  rm -rf "$OUT"; touch $LOCKS/yield_truth   # my extra threads stop: background load as in the reference runs
  st "cb start $TAG $OPP net $NET cfg A book 0 c 8 mt $MT; load $(cut -d' ' -f1 /proc/loadavg)"
  "$MATCH" -a "$REG" -b "exec:$CB \"$DLL\"" --name-a "Regicide-$TAG" "${RX[@]}" "${DBO[@]}" "${OX[@]}" \
    --openings acf156 --movetime $MT -c 8 --pdn-comments -o "$OUT" > "$J/tr-$TAG.log" 2>&1
  rm -f $LOCKS/yield_truth; date +%s > $J/my_last_release
)
st "cb end $TAG $(grep '^A:' $OUT/summary.txt) | $(grep '^Elo (pent' $OUT/summary.txt)"
