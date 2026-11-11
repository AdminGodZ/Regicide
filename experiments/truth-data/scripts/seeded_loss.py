"""Loss of nets on a record file (e.g. the seed-reachable games of the E5 validation tails, from truth startkinds --out),
by piece group, as group_loss.py: BCE of sigmoid(eval/K) against lambda*result + (1-lambda)*sigmoid(score/K), proven
records against their exact result.   python seeded_loss.py FILE NAME=NET... [--k 102.4] [--lam 0.5]"""
import os
import sys

import numpy as np

sys.path.insert(0, "/home/admingod/regicide-truth/tools/nnue")
import nnue  # noqa: E402

GROUPS = ((2, 8), (9, 12), (13, 16), (17, 24))
args = [a for a in sys.argv[1:] if not a.startswith("--")]
K = 102.39766405914318
LAM = 0.5
f, nets = args[0], args[1:]
v = np.asarray(np.memmap(f, nnue.RECORD, "r"))
pc = nnue.piece_counts(v)


def bce(logit, t):
    p = np.clip(1 / (1 + np.exp(-logit)), 1e-12, 1 - 1e-12)
    return float(-np.mean(t * np.log(p) + (1 - t) * np.log(1 - p)))


sets = [v[(pc >= lo) & (pc <= hi)] for lo, hi in GROUPS]
print("%s: %d records; per group %s; K %.1f, lambda %.2f" % (os.path.basename(f), len(v), [len(s) for s in sets], K, LAM))
print("| net | " + " | ".join("%d-%d" % g for g in GROUPS) + " | all (record-weighted) |")
print("|---|" + "---|" * (len(GROUPS) + 1))
for spec in nets:
    name, path = spec.split("=", 1)
    if not os.path.isfile(path):
        sys.exit("no such net %s" % path)
    qw = nnue.read_rgnn(path)
    cells, tot, n = [], 0.0, 0
    for s in sets:
        if len(s) == 0:
            cells.append("-")
            continue
        ev = np.concatenate([nnue.forward_int(qw, s[i:i + 50000]) for i in range(0, len(s), 50000)])
        res = nnue.stm_result(s)
        sc = 1 / (1 + np.exp(-s["score"].astype(np.float64) / K))
        pr = nnue.proven_of(s)
        t = np.where(pr, (s["score"] > 0).astype(np.float64), LAM * res + (1 - LAM) * sc)
        l = bce(ev / K, t)
        cells.append("%.5f" % l)
        tot += l * len(s)
        n += len(s)
    print("| %s | %s | %.5f |" % (name, " | ".join(cells), tot / max(1, n)))
