#!/usr/bin/env bash
# Step 2 of the ponder test: A B C A B C vs Cake (book off), then C with the book on vs Cake, KingsRow, GuiNN.
# Every run takes the timing lock by itself, so other agents' runs interleave. Stop between runs: touch STOP.
J=/mnt/c/regicide-match/jobs/ponder; L=~/regicide-jobs/ponder
run() { [ -e $L/STOP ] && { echo "PT chain stopped before $1 $(date -Is)" >> $L/status; exit 0; }; bash $J/run_p.sh "$@"; }
echo "PT chain start $(date -Is)" >> $L/status
for i in 1 2; do
  run s2-a$i-cake cake A 0
  run s2-b$i-cake cake B 0
  run s2-c$i-cake cake C 0
done
for o in cake kingsrow guinn; do run s2-con-$o $o C 1; done
echo "PT chain done $(date -Is)" >> $L/status
