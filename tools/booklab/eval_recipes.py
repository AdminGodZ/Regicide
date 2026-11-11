#!/usr/bin/env python3
"""WP7c/d: evaluate recipe tests.

  eval_recipes.py --wins wins_census.tsv --test DIR[,DIR] --opp FAMILY [--baseline DIR,DIR...] [--ids w1,w2]
For each recipe whose ballot was played in the test against --opp (default: recipes stolen from --opp):
  stolen colour: Regicide's W/D/L, and how far the games followed the recipe line (plies of agreement after the
  ballot, and whether the win ply was reached);
  other colour: Regicide's W/D/L in the test, and in the baseline runs (same ballot, same colour, same opponent).
Verdict: keep (stolen colour won in every game, other colour no worse than baseline), extend (the win ply was reached
but not every stolen game was won), drop.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402


def games(dirs, opp):
    out = []
    for d in dirs:
        f = os.path.join(d, "games.pdn")
        if not os.path.exists(f):
            continue
        for tags, moves in parse_pdn(open(f, errors="replace").read()):
            bf, wf = family(tags["Black"]), family(tags["White"])
            if "regicide" not in (bf, wf) or opp not in (bf, wf):
                continue
            col = "B" if bf == "regicide" else "W"
            res = tags.get("Result", "")
            r = "D" if res == "1/2-1/2" else ("W" if (res == "1-0") == (col == "B") else "L")
            out.append((ballot_of(tags["Opening"])[1], col, r, [m for m, _ in moves]))
    return out


def wdl(rs):
    c = collections.Counter(rs)
    return f"{c['W']}/{c['D']}/{c['L']}"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--wins", required=True)
    ap.add_argument("--test", required=True)
    ap.add_argument("--opp", required=True)
    ap.add_argument("--baseline", default="")
    ap.add_argument("--ids")
    a = ap.parse_args()
    ids = set(a.ids.split(",")) if a.ids else None
    test = games(a.test.split(","), a.opp)
    base = games([d for d in a.baseline.split(",") if d], a.opp)
    print("id\tballot\tcolour\tsource\tstolen W/D/L\tfollowed (plies after ballot, per game)\twin ply reached\t"
          "other colour W/D/L\tother colour baseline\tverdict")
    for line in open(a.wins):
        f = line.rstrip("\n").split("\t")
        rid, run, rnd, ballot, bmoves, wf, wname, lf, col, nopen, win_ply, nplies, moves = f
        if win_ply == "None" or (ids and rid not in ids) or (not ids and lf != a.opp):
            continue
        win_ply, nopen, rmoves = int(win_ply), int(nopen), moves.split()
        st = [g for g in test if g[0] == bmoves and g[1] == col]
        ot = [g for g in test if g[0] == bmoves and g[1] != col]
        ob = [g for g in base if g[0] == bmoves and g[1] != col]
        if not st:
            continue
        follow = []
        for g in st:
            k = 0
            while k < min(len(g[3]), len(rmoves)) and g[3][k] == rmoves[k]:
                k += 1
            follow.append(k)
        reached = sum(k > win_ply for k in follow)
        sres = [g[2] for g in st]
        worse = sum(r == "L" for _, _, r, _ in ot) > sum(r == "L" for _, _, r, _ in ob) * len(ot) / max(1, len(ob)) \
            if ob else any(g[2] == "L" for g in ot)
        if all(r == "W" for r in sres) and not worse:
            verdict = "keep"
        elif reached and not worse:
            verdict = "extend"
        else:
            verdict = "drop" + (" (other colour worse)" if worse else "")
        print("\t".join(map(str, [rid, ballot, col, f"{wf}>{lf}", wdl(sres),
                                  ",".join(str(k - nopen) for k in follow) + f" (win ply {win_ply - nopen})",
                                  f"{reached}/{len(st)}", wdl([g[2] for g in ot]), wdl([g[2] for g in ob]) if ob else "-",
                                  verdict])))


if __name__ == "__main__":
    main()
