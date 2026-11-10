#!/usr/bin/env bash
# Install the review-round fixes into the worktree, build, and test them on small fixtures.
set -u
S=/mnt/c/regicide-match/jobs/truth/tools; T=~/regicide-truth; X=/tmp/fixes; rm -rf $X; mkdir -p $X
cp $S/datagen.cpp $T/tools/datagen.cpp; cp $S/truth.cpp $T/tools/truth.cpp; cp $S/nnue.py $T/tools/nnue/nnue.py
cp /mnt/c/regicide-match/jobs/truth/e5_gen.sh $T/experiments/truth-data/scripts/jobs/e5_gen.sh
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
cd $T && nice cmake --build build -j 3 --target datagen truth regicide 2>&1 | grep -E " error|warning: " | grep -v store.cpp | head
PY=~/regicide-nnue/.venv/bin/python
echo "== 4. split keeps game starts (two flagged games: results -1 and 0; game 2 starts with a 3-piece record)"
$PY - <<'P'
import sys; sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue"); import nnue, numpy as np
r = np.zeros(4, nnue.RECORD)
r["black"] = [1, 1, 3, 1]; r["white"] = [1 << 31] * 4; r["stm"] = [nnue.GAME_START, 0, nnue.GAME_START, 0]; r["result"] = [-1, -1, 0, 0]
r.tofile("/tmp/fixes/two.bin")
P
./build/truth split --in $X/two.bin --out $X/two_split.bin --min 2 --max 2 > /dev/null
$PY -c "import sys; sys.path.insert(0, '/home/admingod/regicide-truth/tools/nnue'); import nnue, numpy as np; r = np.fromfile('/tmp/fixes/two_split.bin', nnue.RECORD); print('split output: records', len(r), 'game starts', ((r['stm'] & nnue.GAME_START) != 0).astype(int).tolist(), 'results', r['result'].tolist())"
./build/truth relabel --egdb ~/regicide-data/egdb --in $X/two_split.bin --out $X/two_rel.bin | tail -n 1
$PY -c "import sys; sys.path.insert(0, '/home/admingod/regicide-truth/tools/nnue'); import nnue, numpy as np; print('after relabel: results', np.fromfile('/tmp/fixes/two_rel.bin', nnue.RECORD)['result'].tolist())"
echo "== 1. datagen restart with the same seed appends different games"
./build/datagen --games 1 --nodes 2000 --threads 1 --seed 9 --out $X/a.bin > /dev/null
n1=$(( $(stat -c %s $X/a.bin) / 16 ))
./build/datagen --games 1 --nodes 2000 --threads 1 --seed 9 --out $X/a.bin | grep appending
n2=$(( $(stat -c %s $X/a.bin) / 16 ))
cmp -s <(head -c $((n1 * 16)) $X/a.bin) <(tail -c +$((n1 * 16 + 1)) $X/a.bin | head -c $((n1 * 16))) && echo "SAME GAME REPEATED" || echo "blocks differ (first $n1 records, second $((n2 - n1)))"
echo "== 2. datagen refuses a file that ends in a partial record"
head -c $(( n1 * 16 + 5 )) $X/a.bin > $X/p.bin; ./build/datagen --games 1 --nodes 2000 --threads 1 --seed 9 --out $X/p.bin 2>&1 | tail -n 1
echo "== 5. validation cut at a game start (gen6a)"
$PY - <<'P'
import sys; sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue"); import nnue, numpy as np
for f in ["/home/admingod/regicide-data/truth/gen6a_n50k_egdb.bin", "/home/admingod/regicide-data/truth/gen5a_n50k_e2.bin"]:
    m = np.memmap(f, nnue.RECORD, "r")
    s = nnue.valid_start(m, 0.02)
    flagged = bool(int(m[0]["stm"]) & nnue.GAME_START)
    def key(x):
        x = np.asarray(x)
        return (x["black"].astype(np.uint64) * np.uint64(0x9E3779B97F4A7C15)) ^ (x["white"].astype(np.uint64) << np.uint64(29)) ^ (x["kings"].astype(np.uint64) * np.uint64(0xC2B2AE3D27D4EB4F)) ^ (x["stm"].astype(np.uint64) & np.uint64(1))
    kv = key(m[s:]); kt = np.unique(np.concatenate([key(m[i:min(i + 4000000, s)]) for i in range(0, s, 4000000)]))
    both = np.isin(kv, kt)
    print("%s: %d records, flagged %s, index cut %d, validation starts at %d%s; validation positions also in training: %d of %d (%.1f%%)" % (
        f.split("/")[-1], len(m), flagged, len(m) - int(len(m) * 0.02), s,
        " (a game start)" if flagged and int(m[s]["stm"]) & nnue.GAME_START else "", both.sum(), len(kv), 100 * both.mean()))
P
