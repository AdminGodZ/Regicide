#!/usr/bin/env bash
# After gen3a ends: gen3b datagen with 747ab51 (built-in NNUE nn1), seed 43, 300k games, 10k nodes, 12 threads, no egdb.
# Logs file size every 5 min to measure speed.
J=~/regicide-jobs/a44591e
N=~/regicide-nnue-work
D=~/regicide-data/train
until grep -q '^gen3a datagen end' $N/status; do sleep 30; done
while [ -e $J/matchlock ]; do sleep 30; done
BIN=$J/bin/datagen-747ab51
[ -x "$BIN" ] || cp ~/regicide-nnue/build/datagen "$BIN"
OUT=$D/gen3b_n10k.bin
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >> $N/status; exit 1; }
echo "gen3b datagen (747ab51 built-in NNUE nn1) seed 43 threads 12 games 300000 start $(date -Is)" >> $N/status
"$BIN" --games 300000 --nodes 10000 --threads 12 --seed 43 --out "$OUT" > $N/datagen_gen3b.log 2>&1 &
P=$!
while kill -0 $P 2>/dev/null; do echo "$(date +%s) $(stat -c %s $OUT 2>/dev/null || echo 0) $(ps -o stat= -p $P)" >> $N/gen3b_speed.txt; sleep 300; done
wait $P
echo "gen3b datagen end rc=$? $(date -Is)" >> $N/status
