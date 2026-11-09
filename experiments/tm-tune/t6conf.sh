#!/usr/bin/env bash
# Run inside an attached wsl.exe. T6 confirmation (tm_open_pieces=20,tm_open_factor=250), inserted before the deep book:
# holds baselock from now on (the deep book chain waits for matchlock and baselock), waits for the round-6 matches,
# then plays T6 vs Cake again, vs KingsRow and vs GuiNN (312 games each, 0.5 s/move, DLL b88c02e-egdb8, 2-8 EGDB,
# no book/avoid; gen5c alongside), then releases baselock.
J=~/regicide-jobs/r4; R=/mnt/c/regicide-match/jobs/ba/run_ba2.sh
st() { echo "TM $* $(date -Is)" >> $J/status; }
touch $J/baselock
st "T6 confirmation queued (baselock held so the deep book waits)"
until grep -q '^R6N R6N_DONE' $J/status; do sleep 10; done
T6="set param tm_open_pieces=20,tm_open_factor=250"
bash $R b88c02e-egdb8 tm-T6b cake 0.5 "$T6"
bash $R b88c02e-egdb8 tm-T6 kingsrow 0.5 "$T6"
bash $R b88c02e-egdb8 tm-T6 guinn 0.5 "$T6"
rm -f $J/baselock
st "T6CONF_DONE"
