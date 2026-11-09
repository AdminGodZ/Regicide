"""Database-truth metric: on held-out proven positions (9-12 pieces), how often a net's static eval has the sign of
the proven result, and how many proven losses it scores above -30 (and proven wins below +30).

  python db_truth.py --set proven_9_12.bin --net n6c=path/n6c.nnue --net n8a=path/n8a.nnue ...

The eval is the engine's exact integer forward pass (nnue.forward_int), side to move's view. An eval of 0 counts as
not agreeing.
"""
import argparse
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools", "nnue"))
import nnue  # noqa: E402


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--set", action="append", required=True)
    p.add_argument("--net", action="append", required=True, help="NAME=file.nnue")
    p.add_argument("--chunk", type=int, default=50000)
    a = p.parse_args()
    rec = np.concatenate([np.fromfile(f, nnue.RECORD) for f in a.set])
    rec = rec[nnue.proven_of(rec)]
    pieces = nnue.piece_counts(rec)
    sign = np.sign(rec["score"].astype(np.int64))
    print("proven positions %d (wins %d, losses %d); by pieces: %s" % (
        len(rec), (sign > 0).sum(), (sign < 0).sum(),
        ", ".join("%d: %d" % (k, (pieces == k).sum()) for k in range(9, 13))))
    print("| net | sign agrees | 9 | 10 | 11 | 12 | proven losses > -30 | proven wins < +30 | mean eval wins / losses |")
    print("|---|---|---|---|---|---|---|---|---|")
    for spec in a.net:
        name, path = spec.split("=", 1)
        qw = nnue.read_rgnn(path)
        ev = np.concatenate([nnue.forward_int(qw, rec[i:i + a.chunk]) for i in range(0, len(rec), a.chunk)])
        agree = np.sign(ev) == sign
        by = " | ".join("%.2f%%" % (100 * agree[pieces == k].mean()) if (pieces == k).any() else "-"
                        for k in range(9, 13))
        lo = (sign < 0) & (ev > -30)
        wi = (sign > 0) & (ev < 30)
        print("| %s | %.2f%% | %s | %d (%.2f%%) | %d (%.2f%%) | %.0f / %.0f |" % (
            name, 100 * agree.mean(), by, lo.sum(), 100 * lo.sum() / max(1, (sign < 0).sum()), wi.sum(),
            100 * wi.sum() / max(1, (sign > 0).sum()), ev[sign > 0].mean(), ev[sign < 0].mean()))


if __name__ == "__main__":
    sys.exit(main())
