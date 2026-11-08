#!/usr/bin/env bash
# gen3c datagen: master 825f993 (built-in NNUE nn2b), seed 47, 300k games, 10k nodes, 12 threads, no egdb.
J=~/regicide-jobs/r3
D=~/regicide-data/train
mkdir -p $J/bin
BIN=$J/bin/datagen-825f993
[ -x "$BIN" ] || cp ~/regicide-825/build/datagen "$BIN"
OUT=$D/gen3c_n10k.bin
[ -e "$OUT" ] && { echo "refusing: $OUT exists $(date -Is)" >> $J/status; exit 1; }
echo "gen3c datagen (825f993 built-in NNUE nn2b) seed 47 threads 12 games 300000 start $(date -Is)" >> $J/status
"$BIN" --games 300000 --nodes 10000 --threads 12 --seed 47 --out "$OUT" > $J/datagen_gen3c.log 2>&1 &
P=$!
while kill -0 $P 2>/dev/null; do echo "$(date +%s) $(stat -c %s $OUT 2>/dev/null || echo 0) $(ps -o stat= -p $P)" >> $J/gen3c_speed.txt; sleep 300; done
wait $P
echo "gen3c datagen end rc=$? $(date -Is)" >> $J/status
