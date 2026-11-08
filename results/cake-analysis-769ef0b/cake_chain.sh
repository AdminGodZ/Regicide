#!/usr/bin/env bash
# Cake analysis, step 2: lossprobe (master 769ef0b, 2-8 piece EGDB) on the 35 Cake losses of the 769ef0b runs
# (off / avoid / bookavoid at 0.5 s; cake_losses_769ef0b.pdn), after the running 2 s KingsRow run has finished so that
# no CheckerBoard match is disturbed. --movetime 10000 --threads 6 --jobs 2 --hash 1024 (as avoid-v1), gen5a alongside.
J=~/regicide-jobs/r4; JC=~/regicide-jobs/cake; mkdir -p $JC/bin
st() { echo "$* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
B=~/regicide-match/builds/769ef0b-egdb8
nice cmake --build $B/build-linux -j 4 --target lossprobe > $JC/build_lossprobe.log 2>&1 || { st "CAKE lossprobe build failed"; exit 1; }
cp $B/build-linux/lossprobe $JC/bin/lossprobe-769ef0b
{ git -C $B/src log --oneline -1; sha256sum $JC/bin/lossprobe-769ef0b; $B/build-linux/regicide bench 2>&1 | tail -n 1; } > $JC/lossprobe_build.txt
cp /mnt/c/regicide-match/jobs/cake/cake_losses_769ef0b.pdn $JC/
until grep -q '^BA_ALL_769_DONE' $J/status; do sleep 20; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
st "CAKE lossprobe start (35 games, movetime 10000, threads 6, jobs 2, egdb 2-8)"
t0=$(date +%s)
$JC/bin/lossprobe-769ef0b --pdn $JC/cake_losses_769ef0b.pdn --movetime 10000 --threads 6 --jobs 2 --hash 1024 \
  --egdb ~/regicide-data/egdb --out $JC/avoid_cake_769ef0b.bin --report $JC/report_cake_769ef0b.txt > $JC/lossprobe.log 2>&1
st "CAKE lossprobe end rc=$? $(( ($(date +%s) - t0) / 60 )) min: $(tail -n 2 $JC/lossprobe.log | tr '\n' ' ')"
st "CAKE_LOSSPROBE_DONE"
