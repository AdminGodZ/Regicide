"""Which games are duplicated? Copies of a game vs its first record's piece count and game length (gen6a)."""
import hashlib
import sys
from collections import Counter, defaultdict

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

m = np.memmap(sys.argv[1], nnue.RECORD, "r")
starts = np.flatnonzero(np.asarray(m["stm"]) & nnue.GAME_START)
ends = np.append(starts[1:], len(m))
raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
pc = nnue.piece_counts(np.asarray(m[starts]))
copies = Counter()
gh = []
for s, e in zip(starts, ends):
    h = hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()
    copies[h] += 1
    gh.append(h)
by_pc = defaultdict(lambda: [0, 0])
for h, p in zip(gh, pc):
    by_pc[int(p)][0] += 1
    by_pc[int(p)][1] += copies[h] > 1
print("first-record pieces: games, share in a duplicated group")
for p in sorted(by_pc):
    g, d = by_pc[p]
    if g >= 500:
        print("  %2d: %7d games, %5.1f%% duplicated" % (p, g, 100 * d / g))
top = copies.most_common(5)
print("most repeated games:", [c for _, c in top])
lens = ends - starts
dup = np.array([copies[h] > 1 for h in gh])
print("mean length: duplicated games %.1f records, unique games %.1f" % (lens[dup].mean(), lens[~dup].mean()))
