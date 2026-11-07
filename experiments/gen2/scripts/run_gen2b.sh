#!/usr/bin/env bash
# gen2b datagen: a44591e datagen with its built-in eval (w_lr0.03_wd0), seed 37, 300k games, 10k nodes, 12 threads, no egdb.
J=~/regicide-jobs/a44591e
D=~/regicide-data/train
BIN=$J/bin/datagen-a44591e
[ -x "$BIN" ] || cp ~/regicide-gen2/build/datagen "$BIN"
OUT="$D/gen2b_n10k.bin"
[ -e "$OUT" ] && { echo "refusing: $OUT exists" >> $J/status; exit 1; }
echo "gen2b datagen (a44591e built-in) seed 37 threads 12 games 300000 start $(date -Is) pid $$" >> $J/status
"$BIN" --games 300000 --nodes 10000 --threads 12 --seed 37 --out "$OUT" > "$J/datagen_gen2b.log" 2>&1
echo "gen2b datagen end rc=$? $(date -Is)" >> $J/status
