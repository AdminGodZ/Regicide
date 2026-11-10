#!/usr/bin/env bash
# Review item C: what the runner's wall-time accounting cost each engine. Regicide (bl-456e9b2-r10) against Cake,
# KingsRow and GuiNN, 20 games each (the first 10 openings of acf156_h1.txt, both colours), final-validation runner
# match-747ab51, the same options as bl_run_w.sh (incl. set book 3), every engine behind cbshim.py, which logs each
# line with a timestamp. Holds the timing lock per run. Report: cbshim_report.py.
set -u
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
SHIM=$J/src/tools/booklab/cbshim.py
CB='/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe'; E='C:\regicide-match\engines'; DB8='C:\regicide-match\db\kingsrow_wld8'
MATCH=~/regicide-match/bin/match-747ab51
head -10 $J/acf156_h1.txt > $L/acct_deck.txt
st() { echo "$* $(date -Is)" >> $L/status; }
# First: replay any KingsRow mode-3 part that is not finished (k3-kingsrow-a-h1 had 16 crashes at 20:16 from
# exhausted Windows commit; bl_run_w.sh skips finished parts and now guards memory and rejects crashed runs).
for run in a b; do for h in h1 h2; do
  bash $J/bl_run_x.sh regicide:bl-456e9b2-r10 kingsrow k3-kingsrow-$run-$h $J/acf156_$h.txt < /dev/null
done; done
st "K3 replay check done"
for opp in cake kingsrow guinn; do
  case $opp in
    cake) D="$E\\cake\\cake_189f.dll"; X=() ;;
    kingsrow) D="$E\\kingsrow\\Kingsrow64.dll"; X=(--option-b "set searchthreads 1") ;;
    guinn) D="$E\\guinn\\gui_nn64.dll"; X=() ;;
  esac
  OUT=~/regicide-match/results/bl-acct-$opp; LOG=$L/acct/$opp; rm -rf $OUT $LOG; mkdir -p $LOG
  exec 9>~/regicide-jobs/locks/timing.lock; flock 9
  st "BL start acct-$opp (20 games behind cbshim)"
  $MATCH -a "exec:/usr/bin/python3 $SHIM $LOG/A $CB \"$E\\regicide\\bl-456e9b2-r10\\regicide_cb.dll\"" \
         -b "exec:/usr/bin/python3 $SHIM $LOG/B $CB \"$D\"" \
         --option-a "set hashsize 128" --option-a "set threads 1" --option-a "set book 3" \
         --option-b "set dbpath $DB8" --option-b "set enable_wld 1" --option-b "set dbmbytes 512" \
         --option-b "set max_dbpieces 8" --option-b "set hashsize 128" "${X[@]}" --option-b "set book 3" \
         --openings $L/acct_deck.txt --movetime 0.5 -c 8 --pdn-comments -o $OUT > $L/bl-acct-$opp.log 2>&1
  rc=$?
  flock -u 9; exec 9>&-
  st "BL end acct-$opp rc=$rc $(grep '^Games:' $OUT/summary.txt) $(grep '^A:' $OUT/summary.txt)"
done
python3 $J/src/tools/booklab/cbshim_report.py $L/acct ~/regicide-match/results/bl-acct-{cake,kingsrow,guinn} > $L/acct_report.md
st "ACCT done"
cat $L/acct_report.md
