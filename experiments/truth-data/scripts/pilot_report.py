"""E0 pilot report: records with 9-14 pieces re-searched with the 8-piece database at several node budgets.

  python pilot_report.py pilot.csv [--e2dir ~/regicide-data/truth]

A search score is proven when it is decisive (|score| >= 15000: a database win/loss or a forced win by running
the opponent out of moves). For proven records it reports how often the stored (datagen) score has the wrong sign,
and how often the stored game result (and the database-relabelled E2 result, if --e2dir is given) disagrees with
the proven result.
"""
import argparse
import csv
import os
import sys

import numpy as np

KNOWN_WIN = 15000
RECORD = np.dtype([("black", "<u4"), ("white", "<u4"), ("kings", "<u4"), ("score", "<i2"), ("stm", "u1"),
                   ("result", "i1")])


def main():
    p = argparse.ArgumentParser()
    p.add_argument("csv")
    p.add_argument("--e2dir")
    a = p.parse_args()
    rows = list(csv.DictReader(open(a.csv)))
    budgets = [k for k in rows[0] if k.startswith("s") and k[1:].isdigit()]
    file = np.array([r["file"] for r in rows])
    index = np.array([int(r["index"]) for r in rows])
    pieces = np.array([int(r["pieces"]) for r in rows])
    stm = np.array([int(r["stm"]) for r in rows])
    stored = np.array([int(r["stored_score"]) for r in rows])
    result_b = np.array([int(r["result_black"]) for r in rows])
    result_stm = np.where(stm == 0, result_b, -result_b)
    e2_stm = None
    if a.e2dir:
        e2 = np.zeros(len(rows), np.int64)
        for f in np.unique(file):
            m = np.memmap(os.path.join(os.path.expanduser(a.e2dir), f.replace(".bin", "_e2.bin")), RECORD, "r")
            sel = file == f
            e2[sel] = m[index[sel]]["result"]
        e2_stm = np.where(stm == 0, e2, -e2)
    S = {b: np.array([int(r[b]) for r in rows]) for b in budgets}
    deep = np.array(["n50k" in f for f in file])

    print("records %d (%d from 50k-node files)  budgets %s" % (len(rows), deep.sum(), ", ".join(b[1:] for b in budgets)))
    for group, gsel in (("all files", np.ones(len(rows), bool)), ("10k-node files (gen1-gen4a)", ~deep),
                        ("50k-node files (gen5a, gen5c)", deep)):
        print("\n## %s" % group)
        hdr = "| pieces | records |" + "".join(" proven @%s |" % b[1:] for b in budgets)
        last = budgets[-1]
        hdr += " stored score wrong sign (of proven @%s) | stored score 0 | stored result wrong | E2 result wrong |" % last[1:]
        print(hdr)
        print("|" + "---|" * (hdr.count("|") - 1))
        for pc in list(range(9, 15)) + ["9-14"]:
            sel = gsel & ((pieces == pc) if pc != "9-14" else np.ones(len(rows), bool))
            n = sel.sum()
            if n == 0:
                continue
            line = "| %s | %d |" % (pc, n)
            for b in budgets:
                pr = np.abs(S[b][sel]) >= KNOWN_WIN
                line += " %.1f%% |" % (100 * pr.mean())
            pr = sel & (np.abs(S[last]) >= KNOWN_WIN)
            sign = np.sign(S[last])
            np_ = pr.sum()
            wrong = (pr & (stored * sign < 0)).sum()
            zero = (pr & (stored == 0)).sum()
            rwrong = (pr & (result_stm != sign)).sum()
            line += " %.1f%% (%d) | %.1f%% | %.1f%% |" % (100 * wrong / max(1, np_), wrong, 100 * zero / max(1, np_),
                                                           100 * rwrong / max(1, np_))
            if e2_stm is not None:
                line += " %.1f%% |" % (100 * (pr & (e2_stm != sign)).sum() / max(1, np_))
            else:
                line += " - |"
            print(line)

    # Consistency of proofs across budgets: a proof at a small budget against the score at the largest one.
    print("\n## proofs at smaller budgets vs the score at %s nodes" % budgets[-1][1:])
    big = S[budgets[-1]]
    for b in budgets[:-1]:
        pr = np.abs(S[b]) >= KNOWN_WIN
        same = pr & (np.sign(big) == np.sign(S[b]))
        opp = pr & (np.sign(big) == -np.sign(S[b]))
        zero = pr & (big == 0)
        still = pr & (np.abs(big) >= KNOWN_WIN)
        print("- proven @%s: %d; at %s same sign %d, opposite %d, score 0 %d, still proven %d" %
              (b[1:], pr.sum(), budgets[-1][1:], same.sum(), opp.sum(), zero.sum(), still.sum()))
    # How much the score moves when the database is seen (non-proven records): sign changes vs stored.
    print("\n## non-proven records: sign of the new score vs the stored score")
    for b in budgets:
        np_ = np.abs(S[b]) < KNOWN_WIN
        flip = np_ & (stored * S[b] < 0)
        print("- @%s: %d non-proven, opposite sign %d (%.1f%%), mean |new - stored| %.1f" %
              (b[1:], np_.sum(), flip.sum(), 100 * flip.sum() / max(1, np_.sum()),
               np.abs(S[b][np_] - stored[np_]).mean()))


if __name__ == "__main__":
    sys.exit(main())
