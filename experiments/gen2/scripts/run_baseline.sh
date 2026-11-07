#!/usr/bin/env bash
# Baseline re-run: Regicide a44591e (CheckerBoard DLL, 2-7 piece egdb copy on C:) vs KingsRow, Cake 1.89f, GuiNN,
# then KingsRow again with Regicide's book off. Same setup as results/baseline-b71dfa9/README.md.
# Must run inside an attached wsl.exe session (Windows interop), NOT setsid'd.
set -u
MATCH=~/regicide-match/bin/match-a44591e
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
REG="exec:$CB \"$E\regicide\a44591e\regicide_cb.dll\""
DBO=(--option-b "set dbpath $DB8" --option-b "set enable_wld 1" --option-b "set dbmbytes 512" --option-b "set max_dbpieces 8" --option-b "set hashsize 128")
L=~/regicide-jobs/a44591e/match
mkdir -p "$L"
[ -x $MATCH ] || cp ~/regicide-gen2/build/tools/match $MATCH
for run in ${RUNS:-kingsrow cake guinn kingsrow-bookoff}; do
  opp=${run%-bookoff}
  BOOKA=()
  [ "$run" != "$opp" ] && BOOKA=(--option-a "set book 0")
  case $opp in
    kingsrow) DLL="$E\kingsrow\Kingsrow64.dll"; EXTRA=(--option-b "set searchthreads 1") ;;
    cake)     DLL="$E\cake\cake_189f.dll"; EXTRA=() ;;
    guinn)    DLL="$E\guinn\gui_nn64.dll"; EXTRA=() ;;
  esac
  OUT=~/regicide-match/results/${TAG:-baseline}-a44591e-$run
  if [ -f "$OUT/summary.txt" ] && grep -q "Games: 312 of 312" "$OUT/summary.txt"; then echo "skip $run" >> "$L/status"; continue; fi
  echo "start $run $(date -Is)" >> "$L/status"
  "$MATCH" -a "$REG" -b "exec:$CB \"$DLL\"" \
    --option-a "set hashsize 128" --option-a "set threads 1" "${BOOKA[@]}" \
    "${DBO[@]}" "${EXTRA[@]}" \
    --openings acf156 --movetime 0.5 -c 8 --pdn-comments ${MATCH_EXTRA:-} -o "$OUT" > "$L/${TAG:-baseline}-$run.log" 2>&1
  echo "end $run rc=$? $(date -Is)" >> "$L/status"
done
echo "ALL_DONE ${TAG:-baseline} $(date -Is)" >> "$L/status"
