"""Item 4a: on the pilot records (200k random 9-14-piece training records; pilot2.csv, label.hpp rule), the static eval's
sign against the result proven at 200k nodes with the 8-piece database, by piece count, for several nets.
Note: these are training records of every net here (unlike the held-out tails of the database-truth table)."""
import csv
import os
import sys

import numpy as np

sys.path.insert(0, os.path.expanduser("~/regicide-truth/tools/nnue"))
import nnue  # noqa: E402

D = os.path.expanduser("~/regicide-data/train")
W = os.path.expanduser("~/regicide-truth-work")
NETS = [("n6c", os.path.expanduser("~/regicide-nnue6-work/n6c.nnue")), ("n8bn", W + "/n8bn.nnue"),
        ("n8bn2", W + "/n8bn2.nnue"), ("n8c", W + "/n8c.nnue"), ("n8q", W + "/n8q.nnue")]
rows = list(csv.DictReader(open(os.path.expanduser("~/regicide-jobs/truth/pilot2.csv"))))
s = np.array([int(r["s200000"]) for r in rows])
keep = np.abs(s) >= 15000
files = np.array([r["file"] for r in rows])[keep]
index = np.array([int(r["index"]) for r in rows])[keep]
sign = np.sign(s[keep])
rec = np.empty(len(files), nnue.RECORD)
for f in np.unique(files):
    m = np.memmap(os.path.join(D, f), nnue.RECORD, "r")
    sel = files == f
    rec[sel] = m[index[sel]]
pieces = nnue.piece_counts(rec)
ev = {}
for name, path in NETS:
    qw = nnue.read_rgnn(path)
    ev[name] = np.concatenate([nnue.forward_int(qw, rec[i:i + 50000]) for i in range(0, len(rec), 50000)])
print("Pilot records proven at 200k nodes (label.hpp rule): %d (wins %d, losses %d). Share of proven records whose "
      "static eval has the sign of the proven result (eval 0 counts as wrong); in brackets: proven losses scored "
      "above -30 + proven wins scored below +30." % (len(rec), (sign > 0).sum(), (sign < 0).sum()))
print("\n| pieces | proven records | " + " | ".join(n for n, _ in NETS) + " |")
print("|---|---|" + "---|" * len(NETS))
for lo, hi, lab in [(9, 9, "9"), (10, 10, "10"), (11, 11, "11"), (12, 12, "12"), (9, 12, "9-12"), (13, 14, "13-14")]:
    sel = (pieces >= lo) & (pieces <= hi)
    cells = []
    for n, _ in NETS:
        e = ev[n][sel]
        agree = (np.sign(e) == sign[sel]).mean()
        weak = ((sign[sel] < 0) & (e > -30)).sum() + ((sign[sel] > 0) & (e < 30)).sum()
        cells.append("%.2f%% (%d)" % (100 * agree, weak))
    print("| %s | %d | %s |" % (lab, sel.sum(), " | ".join(cells)))
