"""Duplicate-game check for flagged DataRecord files: games are cut at GAME_START; a game is identical to another when
its whole record block (positions, scores, flags, results) is byte-identical. Reports duplicates within each file and
across the files."""
import hashlib
import sys
from collections import Counter

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

seen = Counter()
owner = {}
for f in sys.argv[1:]:
    m = np.memmap(f, nnue.RECORD, "r")
    starts = np.flatnonzero(np.asarray(m["stm"]) & nnue.GAME_START)
    ends = np.append(starts[1:], len(m))
    raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
    within = 0
    hashes = Counter()
    for s, e in zip(starts, ends):
        h = hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()
        hashes[h] += 1
    within = sum(c - 1 for c in hashes.values() if c > 1)
    longest = max((ends - starts)[[i for i, (s, e) in enumerate(zip(starts, ends))][:0]] if False else [0])
    print("%s: %d records, %d games, first record flagged %s; duplicate games within the file: %d" % (
        f.split("/")[-1], len(m), len(starts), bool(starts[0] == 0) if len(starts) else None, within))
    for h, c in hashes.items():
        seen[h] += c
        owner.setdefault(h, set()).add(f)
across = sum(1 for h, fs in owner.items() if len(fs) > 1)
print("games occurring in more than one file: %d; total duplicate games (all copies beyond the first): %d" % (
    across, sum(c - 1 for c in seen.values() if c > 1)))
