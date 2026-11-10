"""Distinct E5 positions: records of the first copy of every game over the files (same rule as dedupe_games.py)."""
import hashlib
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

seen, total, kept, games, kgames = set(), 0, 0, 0, 0
for f in sys.argv[1:]:
    m = np.memmap(f, nnue.RECORD, "r")
    n = len(m) - len(m) % 1
    starts = np.flatnonzero(np.asarray(m["stm"]) & nnue.GAME_START)
    ends = np.append(starts[1:], len(m))
    raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
    for s, e in zip(starts, ends):
        h = hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()
        games += 1
        if h not in seen:
            seen.add(h)
            kept += e - s
            kgames += 1
    total += len(m)
print("records %d, games %d; distinct: %d records in %d games (%.1f%%)" % (total, games, kept, kgames, 100 * kept / total))
