"""Per-phase validation loss of nets: the held-out 2% tails of the given files (>= 9 pieces), by piece-count group.

  python group_loss.py --data F --data G ... --net n6c=a.nnue --net n8bn=b.nnue [--k 102.4] [--lam 0.5] [--per-group 200000]

Loss = binary cross-entropy of sigmoid(eval / K) against lambda * result + (1 - lambda) * sigmoid(score / K) (proven
records: their exact result), as in train.py; also against the result alone (lambda 1) and the score alone (lambda 0).
The eval is the engine's integer eval (nnue.forward_int). Up to --per-group records per group, the same for every net.
"""
import argparse
import os
import sys

import numpy as np

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "tools", "nnue"))
import nnue  # noqa: E402

GROUPS = ((9, 12), (13, 16), (17, 24))


def bce(pred_logit, t):
    p = np.clip(1 / (1 + np.exp(-pred_logit)), 1e-12, 1 - 1e-12)
    return float(-np.mean(t * np.log(p) + (1 - t) * np.log(1 - p)))


def main():
    a = argparse.ArgumentParser()
    a.add_argument("--data", action="append", required=True)
    a.add_argument("--net", action="append", required=True)
    a.add_argument("--k", type=float, default=102.39766405914318)
    a.add_argument("--lam", type=float, default=0.5)
    a.add_argument("--per-group", type=int, default=200000)
    a.add_argument("--seed", type=int, default=1)
    args = a.parse_args()
    d = nnue.Data(sorted(set(args.data)), 0.02, 9)
    v = np.concatenate([np.asarray(p) for p in d.valid_parts])
    pc = nnue.piece_counts(v)
    rng = np.random.default_rng(args.seed)
    sets = []
    for lo, hi in GROUPS:
        idx = np.flatnonzero((pc >= lo) & (pc <= hi))
        if len(idx) > args.per_group:
            idx = np.sort(rng.choice(idx, args.per_group, replace=False))
        sets.append(v[idx])
    K = args.k
    print("validation tails of %d files, >= 9 pieces: %d records; sampled per group %s; K %.1f, lambda %.2f" % (
        len(set(args.data)), len(v), [len(s) for s in sets], K, args.lam))
    print("| net | " + " | ".join("%d-%d λ%.1f | %d-%d result | %d-%d score" % (lo, hi, args.lam, lo, hi, lo, hi)
                                for lo, hi in GROUPS) + " | all λ%.1f (equal weight) |" % args.lam)
    print("|---|" + "---|" * (3 * len(GROUPS) + 1))
    for spec in args.net:
        name, path = spec.split("=", 1)
        qw = nnue.read_rgnn(path)
        cells, mix = [], []
        for s in sets:
            ev = np.concatenate([nnue.forward_int(qw, s[i:i + 50000]) for i in range(0, len(s), 50000)])
            res = nnue.stm_result(s)
            sc = 1 / (1 + np.exp(-s["score"].astype(np.float64) / K))
            pr = nnue.proven_of(s)
            exact = (s["score"] > 0).astype(np.float64)
            t = np.where(pr, exact, args.lam * res + (1 - args.lam) * sc)
            l_mix = bce(ev / K, t)
            cells += ["%.5f" % l_mix, "%.5f" % bce(ev / K, np.where(pr, exact, res)),
                      "%.5f" % bce(ev / K, np.where(pr, exact, sc))]
            mix.append(l_mix)
        print("| %s | %s | %.5f |" % (name, " | ".join(cells), float(np.mean(mix))))


if __name__ == "__main__":
    sys.exit(main())
