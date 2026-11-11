"""Where do the repeated games in the E5 data written since the restart (seeds 616/617) come from?
Each new game (gen6a from record 7,875,584, gen6b from 6,227,456) is classed by its first earlier copy: in the old part
(seeds 611-615), in the new part of the same file, or in the new part of the other file. Also: share of repeated games
that started from a real-game seed vs a random opening is not recorded in the file; we report the start position's
piece count and the game length instead."""
import collections
import hashlib
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

O = "/home/admingod/regicide-data/truth"
NEW = {"gen6a": 7875584, "gen6b": 6227456}


def games(name):
    m = np.memmap("%s/%s_n50k_egdb.bin" % (O, name), nnue.RECORD, "r")
    n = len(m)
    starts = np.flatnonzero(np.asarray(m["stm"]) & nnue.GAME_START)
    ends = np.append(starts[1:], n)
    raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
    for s, e in zip(starts, ends):
        yield s, e, hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()


old = set()
new = {}
for name in NEW:
    for s, e, h in games(name):
        if s < NEW[name]:
            old.add(h)
        else:
            new.setdefault(name, []).append((s, e, h))
first = {}
cls = collections.Counter()
recs = collections.Counter()
lens = collections.defaultdict(list)
# interleave the two new parts by position in file (both generators ran at the same time)
order = sorted([(s - NEW[nm], nm, s, e, h) for nm in new for s, e, h in new[nm]])
for _, nm, s, e, h in order:
    if h in old:
        c = "repeats an old game (seeds 611-615)"
    elif h in first:
        c = "repeats a new game of the same file" if first[h] == nm else "repeats a new game of the other file"
    else:
        c = "first copy"
        first[h] = nm
    cls[c] += 1
    recs[c] += e - s
    lens[c].append(e - s)
tot, totr = sum(cls.values()), sum(recs.values())
print("new games %d, new records %d" % (tot, totr))
for c in cls:
    print("  %-40s games %8d (%5.1f%%)  records %9d (%5.1f%%)  median length %d" % (
        c, cls[c], 100 * cls[c] / tot, recs[c], 100 * recs[c] / totr, int(np.median(lens[c]))))
