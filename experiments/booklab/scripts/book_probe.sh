#!/usr/bin/env bash
# Book-mode probe (lead, review item A1): KingsRow under its default (1) and under set book 3, Cake/GuiNN/Regicide
# set book 3 + read-back. Raw log: ~/regicide-jobs/booklab/book_probe.log. Holds the timing lock.
P=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/book_probe.py
CB=/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe
E='C:\regicide-match\engines'; DB8='C:\regicide-match\db\kingsrow_wld8'
O=~/regicide-jobs/booklab/book_probe.log
OPP=(--opt "set dbpath $DB8" --opt "set enable_wld 1" --opt "set dbmbytes 512" --opt "set max_dbpieces 8" --opt "set hashsize 128")
POS=(--pos "startpos" --pos "startpos moves 9-13 21-17 5-9" --pos "startpos moves 11-15 23-19 8-11" \
     --pos "startpos moves 10-15 21-17 15-18" --pos "startpos moves 11-15 23-19 8-11 22-17")
exec 9>~/regicide-jobs/locks/timing.lock; flock 9
echo "BL start book-probe $(date -Is)" >> ~/regicide-jobs/booklab/status
{
  echo "# book-mode probe $(date -Is); cbhost b71dfa9; match option lists as bl_run.sh"
  echo; echo "## KingsRow, no set book (as all runs so far)"
  python3 $P --cbhost $CB --dll "$E\kingsrow\Kingsrow64.dll" "${OPP[@]}" --opt "set searchthreads 1" "${POS[@]}" --procs 5
  echo; echo "## KingsRow, set book 3"
  python3 $P --cbhost $CB --dll "$E\kingsrow\Kingsrow64.dll" "${OPP[@]}" --opt "set searchthreads 1" --book 3 "${POS[@]}" --procs 5
  echo; echo "## Cake, set book 3"
  python3 $P --cbhost $CB --dll "$E\cake\cake_189f.dll" "${OPP[@]}" --book 3 "${POS[@]}" --procs 2
  echo; echo "## GuiNN, set book 3"
  python3 $P --cbhost $CB --dll "$E\guinn\gui_nn64.dll" "${OPP[@]}" --book 3 "${POS[@]}" --procs 2
  echo; echo "## Regicide bl-456e9b2-r10, set book 3"
  python3 $P --cbhost $CB --dll "$E\regicide\bl-456e9b2-r10\regicide_cb.dll" --opt "set hashsize 128" --opt "set threads 1" --book 3 --pos startpos --pos "startpos moves 9-13 21-17 5-9" --procs 1 --repeat 1
} > $O 2>&1
flock -u 9
echo "BL end book-probe $(date -Is)" >> ~/regicide-jobs/booklab/status
grep -E "^##|^SUMMARY" $O
