#!/usr/bin/env python3
"""Losses of Regicide in given runs: ballot, colour, length, how far Regicide's own book moves went (comments
marked as book by the engine), Regicide's first non-book move, the first ply where its score is <= -100, and the
mode-1 games of the same opening and colour (result, and first difference with the loss).

  k3_losses.py --results DIR --runs bl-k3-kingsrow-a-h1 ... --compare bl-f1-kingsrow-a-h1 ...
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of, is_book  # noqa: E402

SCORE = re.compile(r"(?:score|v|eval)[ =:]?(-?\d+)")


def games(results, runs):
    out = {}
    for r in runs:
        p = os.path.join(results, r, "games.pdn")
        if not os.path.exists(p):
            continue
        for tags, moves in parse_pdn(open(p, errors="replace").read()):
            col = "B" if family(tags["Black"]) == "regicide" else "W"
            res = tags.get("Result", "").strip()
            rr = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
            bm, n = ballot_of(tags["Opening"])[1:]
            out.setdefault((bm, col), []).append(dict(run=r, rnd=tags["Round"], res=rr, moves=moves, nopen=n,
                                                      col=col, ballot=ballot_of(tags["Opening"])[0],
                                                      term=tags.get("Termination", "")))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--compare", nargs="+", default=[])
    a = ap.parse_args()
    g3 = games(a.results, a.runs)
    g1 = games(a.results, a.compare)
    print("| Ballot | Regicide | game | plies | last own book move (ply) | first own search (ply) | own score <= -100 from ply | mode-1 games: results; first difference (ply, by) |")
    print("|---|---|---|---|---|---|---|---|")
    for key, gl in sorted(g3.items(), key=lambda kv: kv[0]):
        for g in gl:
            if g["res"] != "L":
                continue
            own = [i for i in range(g["nopen"], len(g["moves"])) if (i % 2 == 0) == (g["col"] == "B")]
            book = [i for i in own if g["moves"][i][1] and is_book("regicide", g["moves"][i][1])]
            first_search = next((i for i in own if not (g["moves"][i][1] and is_book("regicide", g["moves"][i][1]))), None)
            bad = None
            for i in own:
                c = g["moves"][i][1] or ""
                m = re.search(r"\bscore (-?\d+)", c)
                if m and int(m.group(1)) <= -100:
                    bad = i + 1
                    break
            cmp = []
            for h in g1.get(key, []):
                mv3, mv1 = [m for m, _ in g["moves"]], [m for m, _ in h["moves"]]
                d = next((i for i in range(min(len(mv3), len(mv1))) if mv3[i] != mv1[i]), None)
                by = "-" if d is None else ("Regicide" if (d % 2 == 0) == (g["col"] == "B") else "KingsRow")
                cmp.append(f"{h['res']}; {'identical' if d is None else f'ply {d + 1} by {by}'}")
            print(f"| {g['ballot']} | {g['col']} | {g['run']} r{g['rnd']} | {len(g['moves'])} | "
                  f"{(book[-1] + 1) if book else '-'} | {(first_search + 1) if first_search is not None else '-'} | "
                  f"{bad or '-'} | {' / '.join(cmp) or '-'} |")


if __name__ == "__main__":
    main()
