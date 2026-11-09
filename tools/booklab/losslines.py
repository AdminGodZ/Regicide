#!/usr/bin/env python3
"""List Regicide's losses (or wins) in match result dirs, grouped by ballot, colour and line.

  losslines.py [--wins] [--plies 30] [--name Regicide] DIR...
For every lost game: ballot, Regicide's colour, the moves after the ballot (first --plies plies). Lines are grouped
and the first ply where losing games of one ballot/colour diverge is shown.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--wins", action="store_true")
    ap.add_argument("--plies", type=int, default=30)
    ap.add_argument("--fam", default="regicide")
    ap.add_argument("dirs", nargs="+")
    a = ap.parse_args()
    groups = collections.defaultdict(list)
    totals = collections.Counter()
    for d in a.dirs:
        f = os.path.join(d, "games.pdn") if os.path.isdir(d) else d
        for tags, moves in parse_pdn(open(f, errors="replace").read()):
            bf, wf = family(tags.get("Black", "")), family(tags.get("White", ""))
            if a.fam not in (bf, wf):
                continue
            col = "B" if bf == a.fam else "W"
            opp = wf if col == "B" else bf
            op = tags.get("Opening", "")
            ballot, _, nopen = ballot_of(op)
            res = tags.get("Result", "")
            won = (res == "1-0" and col == "B") or (res == "0-1" and col == "W")
            lost = (res == "1-0" and col == "W") or (res == "0-1" and col == "B")
            totals[(ballot, col, opp)] += 1
            if (a.wins and won) or (not a.wins and lost):
                line = " ".join(m for m, _ in moves[nopen:nopen + a.plies])
                groups[(ballot, col, opp)].append((os.path.basename(os.path.dirname(f)), tags.get("Round", ""), line))
    for (ballot, col, opp), gs in sorted(groups.items(), key=lambda kv: -len(kv[1])):
        print(f"== {ballot} Regicide {col} vs {opp}: {len(gs)} {'wins' if a.wins else 'losses'} of "
              f"{totals[(ballot, col, opp)]} games")
        lines = collections.Counter(g[2] for g in gs)
        for line, n in lines.most_common():
            srcs = sorted({g[0] for g in gs if g[2] == line})
            print(f"  {n:3d}x  {line}   [{', '.join(srcs)[:120]}]")


if __name__ == "__main__":
    main()
