#!/usr/bin/env bash
# Run inside an attached wsl.exe. Cake analysis, step 3: Cake at 2 s/move, 312 games (acf156), DLL 769ef0b-egdb8
# (no book, no avoid list, 2-8 EGDB, default cache), after lossprobe so that nothing else uses the CPU besides gen5a
# (12 threads, as in the baselines). Then the 0.5 s off-vs-Cake repeat under the gen5 background (off-769b).
J=~/regicide-jobs/r4; R=/mnt/c/regicide-match/jobs/ba/run_ba2.sh
st() { echo "$* $(date -Is)" >> $J/status; }
until grep -q '^CAKE_LOSSPROBE_DONE' $J/status; do sleep 30; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
bash $R 769ef0b-egdb8 off-769-2s cake 2
st "CAKE_2S_DONE"
bash $R 769ef0b-egdb8 off-769b cake 0.5
st "CAKE_ALL_DONE"
