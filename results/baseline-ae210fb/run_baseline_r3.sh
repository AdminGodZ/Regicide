#!/usr/bin/env bash
# Round-3 baseline: Regicide CheckerBoard DLL engines/regicide/$NAME (2-7 piece egdb copy) vs KingsRow, Cake 1.89f,
# GuiNN. Same settings as results/baseline-747ab51 (acf156, 312 games, 0.5 s/move, -c 8, hash 128, threads 1, no book).
# Must run inside an attached wsl.exe session (Windows interop), NOT setsid'd.
set -u
NAME=${NAME:?}
MATCH=~/regicide-match/bin/match-747ab51     # tools/match and tools/cbhost unchanged since b71dfa9
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'
E='C:\regicide-match\engines'
DB8='C:\regicide-match\db\kingsrow_wld8'
REG="exec:$CB \"$E\regicide\\$NAME\regicide_cb.dll\""
DBO=(--option-b "set dbpath $DB8" --option-b "set enable_wld 1" --option-b "set dbmbytes 512" --option-b "set max_dbpieces 8" --option-b "set hashsize 128")
L=~/regicide-jobs/r3
st() { echo "$* $(date -Is)" >> $L/status; }
for run in ${RUNS:-kingsrow cake guinn kingsrow-book-w03}; do
  # kingsrow-book-w03: the same engine with experiments/book-w03/book_w03.bin embedded (DLL $NAME-book-w03),
  # default CB book mode (3 = best moves only)
  REG="exec:$CB \"$E\regicide\\$NAME\regicide_cb.dll\""
  [ $run = kingsrow-book-w03 ] && REG="exec:$CB \"$E\regicide\\$NAME-book-w03\regicide_cb.dll\""
  case $run in
    kingsrow*) DLL="$E\kingsrow\Kingsrow64.dll"; EXTRA=(--option-b "set searchthreads 1") ;;
    cake)     DLL="$E\cake\cake_189f.dll"; EXTRA=() ;;
    guinn)    DLL="$E\guinn\gui_nn64.dll"; EXTRA=() ;;
  esac
  OUT=~/regicide-match/results/baseline-$NAME-$run
  if [ -f "$OUT/summary.txt" ] && grep -q "Games: 312 of 312" "$OUT/summary.txt"; then st "BASELINE skip $run"; continue; fi
  st "BASELINE start $NAME $run"
  "$MATCH" -a "$REG" -b "exec:$CB \"$DLL\"" \
    --option-a "set hashsize 128" --option-a "set threads 1" \
    "${DBO[@]}" "${EXTRA[@]}" \
    --openings acf156 --movetime 0.5 -c 8 --pdn-comments -o "$OUT" > "$L/baseline-$NAME-$run.log" 2>&1
  st "BASELINE end $NAME $run rc=$? $(grep '^Elo (pent' $OUT/summary.txt)"
done
st "BASELINE_ALL_DONE $NAME"
