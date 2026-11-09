#!/usr/bin/env python3
"""WP5a driver: for every (ballot, colour, opponent) Regicide lost in the given result dirs, run `theory.py repair` on
the most recent such game and write the candidate force decisions to OUTDIR/<ballot>_<colour>_<opp>.txt.

  repair_all.py --db theory.sqlite --out OUTDIR [--follow 40] RESULTDIR...
"""
import argparse
import contextlib
import io
import os
import re
import sqlite3
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import theory  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--db", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--follow", type=int, default=40)
    ap.add_argument("--min-pieces", type=int, default=8)
    ap.add_argument("dirs", nargs="+")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    db = sqlite3.connect(a.db)
    latest = {}
    for d in a.dirs:
        name = os.path.basename(d.rstrip("/"))
        for gid, ballot, bfam, wfam, result in db.execute(
                "SELECT gid, ballot, bfam, wfam, result FROM games WHERE src LIKE ? AND "
                "((bfam='regicide' AND result='W') OR (wfam='regicide' AND result='B'))", (f"%/{name}/games.pdn",)):
            col = "B" if bfam == "regicide" else "W"
            opp = wfam if col == "B" else bfam
            latest[(ballot, col, opp)] = max(gid, latest.get((ballot, col, opp), 0))
    for (ballot, col, opp), gid in sorted(latest.items()):
        slug = re.sub(r"[^A-Za-z0-9]+", "_", ballot).strip("_") + f"_{col}_{opp}"
        buf = io.StringIO()
        with contextlib.redirect_stdout(buf):
            theory.repair(argparse.Namespace(db=a.db, gid=gid, fam="regicide", opp=opp, max_search=60,
                                             follow=a.follow, min_pieces=a.min_pieces, pin_path=False))
        open(os.path.join(a.out, slug + ".txt"), "w").write(buf.getvalue())
        dev = [l for l in buf.getvalue().splitlines() if l.startswith("# DEVIATION") or l.startswith("# no deviation")]
        n = sum(1 for l in buf.getvalue().splitlines() if l and not l.startswith("#"))
        print(f"{slug}: gid {gid}; {dev[0][2:] if dev else '?'}; {n} force lines")


if __name__ == "__main__":
    main()
