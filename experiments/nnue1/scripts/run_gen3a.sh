#!/usr/bin/env bash
# gen3a datagen: bd2ef1d datagen with its built-in eval (G2c), seed 41, 300k games, 10k nodes, 12 threads, no egdb.
J=~/regicide-jobs/a44591e
N=~/regicide-nnue-work
D=~/regicide-data/train
BIN=$J/bin/datagen-bd2ef1d
[ -x "$BIN" ] || cp ~/regicide-nnue/build/datagen "$BIN"
OUT="$D/gen3a_n10k.bin"
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >> $N/status; exit 1; }
echo "gen3a datagen (bd2ef1d built-in = G2c) seed 41 threads 12 games 300000 start $(date -Is) pid $$" >> $N/status
"$BIN" --games 300000 --nodes 10000 --threads 12 --seed 41 --out "$OUT" > "$N/datagen_gen3a.log" 2>&1
echo "gen3a datagen end rc=$? $(date -Is)" >> $N/status
