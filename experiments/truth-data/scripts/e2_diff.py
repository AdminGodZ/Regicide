"""Compare an old E2 file with its rebuilt E2f file record by record: board, score and flags must be identical, only
results may differ, and only on records with <= 8 pieces. Prints the changed results by piece count."""
import os
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

a = np.memmap(sys.argv[1], nnue.RECORD, "r")
b = np.memmap(sys.argv[2], nnue.RECORD, "r")
name = os.path.basename(sys.argv[2])
if len(a) != len(b):
    sys.exit("%s: %d records against %d" % (name, len(a), len(b)))
changed = np.zeros(25, np.int64)
other = 0
for s in range(0, len(a), 4_000_000):
    x, y = np.asarray(a[s:s + 4_000_000]), np.asarray(b[s:s + 4_000_000])
    for k in ("black", "white", "kings", "score", "stm"):
        other += int(np.count_nonzero(x[k] != y[k]))
    d = x["result"] != y["result"]
    pc = nnue.piece_counts(y[d])
    changed += np.bincount(pc, minlength=25)
above = int(changed[9:].sum())
print("%s: records %d; results changed %d (<= 8 pieces %d, > 8 pieces %d); other fields differing %d%s" % (
    name, len(b), int(changed.sum()), int(changed[:9].sum()), above, other,
    "" if other == 0 and above == 0 else "  CHECK"))
print("  by pieces 2-8: %s" % " ".join("%d:%d" % (p, changed[p]) for p in range(2, 9)))
