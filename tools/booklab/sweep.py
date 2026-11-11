#!/usr/bin/env python3
"""Preventive sweep (WP5 extension): positions the CURRENT Regicide reaches (its moves in the given result dirs) where
  - Regicide's move has a losing record in the theory table (all Regicide versions; >= --min-losses losses and a
    loss rate >= --min-rate), and
  - the top engines (Cake, KingsRow, GuiNN) played another move there at least --min-top times in total and never
    lost with it, and never played Regicide's move without losing.
Prints force decisions (the top engines' most played non-losing move) with the evidence.

  sweep.py --db theory.sqlite [--max-after 40] RESULTDIR...
"""
import argparse
import collections
import os
import sqlite3
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import side_at  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--db", required=True)
    ap.add_argument("--max-after", type=int, default=40)
    ap.add_argument("--min-losses", type=int, default=2)
    ap.add_argument("--min-rate", type=float, default=0.2)
    ap.add_argument("--min-top", type=int, default=5)
    ap.add_argument("dirs", nargs="+")
    a = ap.parse_args()
    db = sqlite3.connect(a.db)
    seen = {}
    for d in a.dirs:
        name = os.path.basename(d.rstrip("/"))
        for gid, ballot, bfam, wfam, fen, nopen in db.execute(
                "SELECT gid, ballot, bfam, wfam, fen, nopen FROM games WHERE src LIKE ?", (f"%/{name}/games.pdn",)):
            col = "B" if bfam == "regicide" else ("W" if wfam == "regicide" else None)
            if col is None:
                continue
            for ply, key, mv in db.execute("SELECT ply, key, move FROM plies WHERE gid=? AND ply>=? AND ply<?",
                                           (gid, nopen, nopen + a.max_after)):
                if side_at(fen, ply) == col:
                    seen.setdefault((key, mv), (ballot, col, ply, gid))
    out = []
    for (key, mv), (ballot, col, ply, gid) in seen.items():
        rows = db.execute("SELECT fam, move, n, w, d, l FROM theory WHERE key=?", (key,)).fetchall()
        reg = [r for r in rows if r[0] == "regicide" and r[1] == mv]
        if not reg or reg[0][5] < a.min_losses or reg[0][5] / reg[0][2] < a.min_rate:
            continue
        top = collections.defaultdict(lambda: [0, 0])
        for fam, m, n, w, d, l in rows:
            if fam in ("cake", "kingsrow", "guinn"):
                top[m][0] += n
                top[m][1] += l
        if mv in top and top[mv][1] == 0:
            continue  # the top engines play our move too and never lost with it
        good = [(m, v[0]) for m, v in top.items() if v[1] == 0 and m != mv]
        if not good:
            continue
        best = max(good, key=lambda x: x[1])
        if best[1] < a.min_top:
            continue
        fen = db.execute("SELECT fen FROM positions WHERE key=?", (key,)).fetchone()[0]
        detail = "; ".join(f"{fam} {m} {n}x w/d/l {w}/{d}/{l}" for fam, m, n, w, d, l in rows
                           if fam in ("cake", "kingsrow", "guinn"))
        out.append((ballot, ply, f"{fen} | {best[0]} | force | sweep {ballot} regicide {col} ply {ply}: regicide "
                    f"played {mv} {reg[0][2]}x w/d/l {reg[0][3]}/{reg[0][4]}/{reg[0][5]} (all versions; current "
                    f"version in gid {gid}); top engines: {detail}"))
    for _, _, line in sorted(out):
        print(line)
    print(f"# {len(out)} sweep decisions from {len(seen)} (position, move) pairs of the current version",
          file=sys.stderr)


if __name__ == "__main__":
    main()
