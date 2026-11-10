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
    p.add_argument("--general", help="DataRecord file: also report mean |eval| on its >= 9-piece records (eval scale)")
    p.add_argument("--general-n", type=int, default=200000)
    p.add_argument("--dump", help="write the per-record evals (gzip CSV: set, index in set, pieces, proven sign, eval per net)")
    a = p.parse_args()
    parts = [np.fromfile(f, nnue.RECORD) for f in a.set]
    src = np.concatenate([np.full(len(x), i) for i, x in enumerate(parts)])
    idx = np.concatenate([np.arange(len(x)) for x in parts])
    rec = np.concatenate(parts)
    keep = nnue.proven_of(rec)
    rec, src, idx = rec[keep], src[keep], idx[keep]
    dump = {}
    pieces = nnue.piece_counts(rec)
    sign = np.sign(rec["score"].astype(np.int64))
    print("proven positions %d (wins %d, losses %d); by pieces: %s" % (
        len(rec), (sign > 0).sum(), (sign < 0).sum(),
        ", ".join("%d: %d" % (k, (pieces == k).sum()) for k in range(9, 13))))
    gen = None
    if a.general:
        g = np.fromfile(a.general, nnue.RECORD)
        g = g[nnue.piece_counts(g) >= 9][: a.general_n]
        gen = g
    print("| net | sign agrees | 9 | 10 | 11 | 12 | proven losses > -30 | proven wins < +30 | mean eval wins / losses |"
          + (" mean abs eval (>= 9 pieces, %d general positions) |" % len(gen) if gen is not None else ""))
    print("|---|---|---|---|---|---|---|---|---|" + ("---|" if gen is not None else ""))
    for spec in a.net:
        name, path = spec.split("=", 1)
        qw = nnue.read_rgnn(path)
        ev = np.concatenate([nnue.forward_int(qw, rec[i:i + a.chunk]) for i in range(0, len(rec), a.chunk)])
        dump[name] = ev
        agree = np.sign(ev) == sign
        by = " | ".join("%.2f%%" % (100 * agree[pieces == k].mean()) if (pieces == k).any() else "-"
                        for k in range(9, 13))
        lo = (sign < 0) & (ev > -30)
        wi = (sign > 0) & (ev < 30)
        print("| %s | %.2f%% | %s | %d (%.2f%%) | %d (%.2f%%) | %.0f / %.0f |" % (
            name, 100 * agree.mean(), by, lo.sum(), 100 * lo.sum() / max(1, (sign < 0).sum()), wi.sum(),
            100 * wi.sum() / max(1, (sign > 0).sum()), ev[sign > 0].mean(), ev[sign < 0].mean())
            + ("" if gen is None else " %.1f |" % np.abs(np.concatenate(
                [nnue.forward_int(qw, gen[i:i + a.chunk]) for i in range(0, len(gen), a.chunk)])).mean()))

    if a.dump:
        import gzip
        with gzip.open(a.dump, "wt") as f:
            f.write("set,index,pieces,proven_sign," + ",".join(dump) + "\n")
            for i in range(len(rec)):
                f.write("%s,%d,%d,%d,%s\n" % (os.path.basename(a.set[src[i]]), idx[i], pieces[i], sign[i],
                                              ",".join(str(int(dump[n][i])) for n in dump)))


if __name__ == "__main__":
    sys.exit(main())
