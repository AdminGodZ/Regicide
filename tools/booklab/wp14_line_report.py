#!/usr/bin/env python3
"""WP14: results, lowest own scores and opponent lines of one opening and colour, per group of runs.

  wp14_line_report.py --opening "ACF #6 Inferno" --colour B --group NAME=RUNDIR,RUNDIR ... [--from-ply 17] [--to-ply 40]
                      [--md OUT]
Per group: W / D / L; our lowest search score per game (Regicide's PDN comment "score N"; a database or mate score,
|N| >= 9000, is shown as "lost (db)" / "won (db)"), as a distribution (min, quartiles, max) and in bands; the number of
distinct move sequences from FROM-PLY to TO-PLY (both sides, games shorter than TO-PLY to their end) and of distinct
opponent moves at the first ply where the games of the group part; and which games reached a given ply.
"""
import argparse
import collections
import os
import re
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of  # noqa: E402


def lowest(moves, col):
    best = None
    for i, (_, c) in enumerate(moves):
        if (i % 2 == 0) == (col == "B"):
            s = re.search(r"\bscore (-?\d+)", c or "")
            if s:
                v = int(s.group(1))
                best = v if best is None else min(best, v)
    return best


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--opening", required=True)
    ap.add_argument("--colour", required=True)
    ap.add_argument("--group", nargs="+", required=True)
    ap.add_argument("--from-ply", type=int, default=17)
    ap.add_argument("--to-ply", type=int, default=40)
    ap.add_argument("--md")
    a = ap.parse_args()
    out = [f"| Group | games | W / D / L | lowest own score: min / q1 / median / q3 / max | ≤ −100 | −99 … −60 | −59 … −30 | > −30 | "
           f"distinct lines, plies {a.from_ply}-{a.to_ply} |", "|---|---|---|---|---|---|---|---|---|"]
    for spec in a.group:
        name, dirs = spec.split("=", 1)
        gs = []
        for d in dirs.split(","):
            p = os.path.join(d, "games.pdn")
            if not os.path.exists(p):
                continue
            for t, m in parse_pdn(open(p, errors="replace").read()):
                if ballot_of(t.get("Opening", ""))[0] != a.opening:
                    continue
                col = "B" if t["Black"].lower().startswith("regicide") else "W"
                if col != a.colour:
                    continue
                r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(t.get("Result", ""), "?")
                gs.append(dict(r=r, low=lowest(m, col), line=tuple(x for x, _ in m[a.from_ply - 1:a.to_ply])))
        if not gs:
            out.append(f"| {name} | 0 | | | | | | | |")
            continue
        c = collections.Counter(g["r"] for g in gs)
        lows = sorted(g["low"] for g in gs if g["low"] is not None and abs(g["low"]) < 9000)
        db_lost = sum(1 for g in gs if g["low"] is not None and g["low"] <= -9000)
        q = statistics.quantiles(lows, n=4) if len(lows) >= 2 else [lows[0]] * 3 if lows else [None] * 3
        band = lambda lo, hi: sum(1 for v in lows if lo <= v <= hi)
        dist = f"{lows[0]} / {q[0]:.0f} / {q[1]:.0f} / {q[2]:.0f} / {lows[-1]}" if lows else "—"
        out.append(f"| {name} | {len(gs)} | {c['W']} / {c['D']} / {c['L']} | {dist}" + (f" (+{db_lost} lost (db))" if db_lost else "")
                   + f" | {band(-8999, -100) + db_lost} | {band(-99, -60)} | {band(-59, -30)} | {band(-29, 99999)} | "
                   f"{len(set(g['line'] for g in gs))} |")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
