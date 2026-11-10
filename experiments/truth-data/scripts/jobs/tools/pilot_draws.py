"""Item 4a addition: draws at 9-12 pieces. A search cannot prove a draw above the database (8 pieces), so the pilot has no
proven draws at 9-12 pieces. Proxy (no new searches): pilot records (re-run, label.hpp rule) NOT proven at 200k nodes
whose game's database value at its first <= 8-piece record is a draw (the E2 relabelled result); stricter: also
|200k-node score| <= 20. Per piece count and material balance: mean |static eval| and share within +-100. For proven
wins/losses: share scored beyond +-100 in the right direction."""
import csv
import os
import sys

import numpy as np

sys.path.insert(0, os.path.expanduser("~/regicide-truth/tools/nnue"))
import nnue  # noqa: E402

D = os.path.expanduser("~/regicide-data/train")
E2 = os.path.expanduser("~/regicide-data/truth")
W = os.path.expanduser("~/regicide-truth-work")
NETS = [("n6c", os.path.expanduser("~/regicide-nnue6-work/n6c.nnue")), ("n8bn", W + "/n8bn.nnue"), ("n8q", W + "/n8q.nnue")]
if len(sys.argv) > 1:  # NAME=net.nnue arguments replace the default nets
    NETS = [tuple(a.split("=", 1)) for a in sys.argv[1:]]
rows = list(csv.DictReader(open(os.path.expanduser("~/regicide-jobs/truth/pilot2.csv"))))
files = np.array([r["file"] for r in rows])
index = np.array([int(r["index"]) for r in rows])
s = np.array([int(r["s200000"]) for r in rows])
rec = np.empty(len(rows), nnue.RECORD)
e2res = np.zeros(len(rows), np.int64)
for f in np.unique(files):
    sel = files == f
    rec[sel] = np.memmap(os.path.join(D, f), nnue.RECORD, "r")[index[sel]]
    e2res[sel] = np.memmap(os.path.join(E2, f.replace(".bin", "_e2.bin")), nnue.RECORD, "r")[index[sel]]["result"]
pieces = nnue.piece_counts(rec)
stm = nnue.stm_of(rec)
nb, nw = np.bitwise_count(rec["black"]), np.bitwise_count(rec["white"])
bal = np.where(stm == 0, nb.astype(int) - nw, nw.astype(int) - nb)  # side to move minus opponent, kings count 1
proven = np.abs(s) >= 15000
ev = {n: np.concatenate([nnue.forward_int(nnue.read_rgnn(p), rec[i:i + 50000]) for i in range(0, len(rec), 50000)])
      for n, p in NETS}
names = [n for n, _ in NETS]
draw = ~proven & (e2res == 0)
strict = draw & (np.abs(s) <= 20)
print("Pilot records with 9-12 pieces: %d; proven at 200k: %d (all wins or losses: a search cannot prove a draw above "
      "the database); proven draws: 0." % (((pieces >= 9) & (pieces <= 12)).sum(), (proven & (pieces <= 12)).sum()))
for title, mask in (("Draw proxy: not proven at 200k, game's database value a draw", draw),
                    ("Stricter draw proxy: as above and |200k score| <= 20", strict)):
    print("\n**%s** - mean |eval| / share within ±100:\n" % title)
    print("| pieces | material (stm − opp) | records | " + " | ".join(names) + " |")
    print("|---|---|---|" + "---|" * len(names))
    for pc in (9, 10, 11, 12):
        for blab, bsel in (("equal", bal == 0), ("±1", np.abs(bal) == 1), ("all", np.ones(len(rows), bool))):
            sel = mask & (pieces == pc) & bsel
            if sel.sum() == 0:
                continue
            cells = ["%.0f / %.0f%%" % (np.abs(ev[n][sel]).mean(), 100 * (np.abs(ev[n][sel]) <= 100).mean()) for n in names]
            print("| %d | %s | %d | %s |" % (pc, blab, sel.sum(), " | ".join(cells)))
print("\n**Proven wins / losses** - share scored beyond ±100 in the proven direction:\n")
print("| pieces | proven | " + " | ".join(names) + " |")
print("|---|---|" + "---|" * len(names))
for pc in (9, 10, 11, 12):
    sel = proven & (pieces == pc)
    cells = ["%.1f%%" % (100 * (np.sign(s[sel]) * ev[n][sel] > 100).mean()) for n in names]
    print("| %d | %d | %s |" % (pc, sel.sum(), " | ".join(cells)))
