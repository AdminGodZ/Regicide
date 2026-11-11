"""Validation records per input file of the n9h recipe (valid_frac 0.02, min pieces 2), with the rule of nnue.Data.
E5: the raw gen6a/gen6b files stand in for their game-deduplicated copies (n9.sh makes those when it starts; train.py
now prints the exact counts in the n9h log)."""
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

O = "/home/admingod/regicide-data/truth"
files = ["%s/%s_n10k_e2.bin" % (O, f) for f in "gen1 gen1b gen1c gen2a gen2b gen3a gen3b gen3c gen3d gen4a".split()]
files += ["%s/gen5a_n50k_e2.bin" % O] + sys.argv[1:]
for f in files:
    m = np.memmap(f, nnue.RECORD, "r")
    s = nnue.valid_start(m, 0.02)
    k = nnue.kept_indices(m, 2)
    nv = len(k) - int(np.searchsorted(k, s))
    flagged = bool(len(m) and int(m[0]["stm"]) & nnue.GAME_START)
    print("%-60s records %10d  flagged %-5s  index cut %10d  validation from %10d  validation records (>= 2 pieces) %8d%s" % (
        f.split("/")[-1], len(m), flagged, len(m) - int(len(m) * 0.02), s, nv, "  NONE" if nv == 0 else ""))
