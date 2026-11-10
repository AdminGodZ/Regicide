"""Write copies of flagged DataRecord files with every repeated game removed (a game = the block from one GAME_START to
the next; identical = byte-identical block). The first occurrence over all files, in the order given, is kept.

  python dedupe_games.py OUTDIR FILE...    -> OUTDIR/<name>_dedup.bin, and a summary line per file
"""
import hashlib
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools", "nnue"))
sys.path.insert(0, os.path.expanduser("~/regicide-truth/tools/nnue"))
import nnue  # noqa: E402


def main():
    outdir, files = sys.argv[1], sys.argv[2:]
    seen = set()
    for f in files:
        m = np.memmap(f, nnue.RECORD, "r")
        flags = np.asarray(m["stm"]) & nnue.GAME_START
        if len(m) == 0 or not flags[0]:
            sys.exit("%s: not a flagged file" % f)
        starts = np.flatnonzero(flags)
        ends = np.append(starts[1:], len(m))
        raw = np.asarray(m).view(np.uint8).reshape(-1, 16)
        keep = []
        for s, e in zip(starts, ends):
            h = hashlib.blake2b(raw[s:e].tobytes(), digest_size=16).digest()
            if h not in seen:
                seen.add(h)
                keep.append((s, e))
        out = os.path.join(outdir, os.path.basename(f).replace(".bin", "_dedup.bin"))
        with open(out + ".tmp", "wb") as o:
            for s, e in keep:
                o.write(raw[s:e].tobytes())
        os.replace(out + ".tmp", out)
        kept = sum(e - s for s, e in keep)
        print("%s: %d games, %d kept (%d records of %d) -> %s" % (os.path.basename(f), len(starts), len(keep), kept,
                                                                len(m), os.path.basename(out)))


if __name__ == "__main__":
    main()
