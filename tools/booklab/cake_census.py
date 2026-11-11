#!/usr/bin/env python3
"""Census of Cake's play by opening (lead 2026-11-11, WP12): over all our production-deck games against Cake.

  cake_census.py --runs RUNDIR ... [--plies 20] [--score 60] [--md OUT]
For every opening and Regicide colour: games, Cake's distinct lines (the move sequence of both sides from the end of the
ballot up to ply PLIES; our moves are mostly book moves, so different lines are almost always Cake's choice), the
ply where Cake's lines first part, our results, and our best own search score (Regicide's PDN comment "score N",
mate/database scores above 9000 left out). Lists: openings where Cake varies most; openings where any game ended in our
win or our score rose above SCORE.
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--plies", type=int, default=20)
    ap.add_argument("--score", type=int, default=60)
    ap.add_argument("--md")
    a = ap.parse_args()
    by = collections.defaultdict(list)  # (ballot name, moves, colour) -> games
    for d in sorted(a.runs):
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            rb = tags.get("Black", "").strip().lower().startswith("regicide")
            rw = tags.get("White", "").strip().lower().startswith("regicide")
            other = (tags.get("White") if rb else tags.get("Black", "")) or ""
            if rb == rw or "cake" not in other.lower():
                continue
            col = "B" if rb else "W"
            res = tags.get("Result", "").strip()
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
            b = ballot_of(tags["Opening"])
            mv = [m for m, _ in moves]
            best = None
            for i, (m, c) in enumerate(moves):
                if (i % 2 == 0) == (col == "B"):
                    s = re.search(r"\bscore (-?\d+)", c or "")
                    if s and abs(int(s.group(1))) < 9000:
                        best = int(s.group(1)) if best is None else max(best, int(s.group(1)))
            by[(b[0], b[1], col)].append(dict(res=r, line=tuple(mv[b[2]:a.plies]), best=best,
                                              ref=f"{os.path.basename(d.rstrip('/'))} {tags.get('Round', '?')}"))
    rows = []
    for (name, bm, col), gs in by.items():
        lines = collections.Counter(g["line"] for g in gs)
        part = None
        if len(lines) > 1:
            ls = list(lines)
            part = min(next((i for i in range(min(len(x), len(y))) if x[i] != y[i]), min(len(x), len(y)))
                       for j, x in enumerate(ls) for y in ls[j + 1:]) + len(bm.split()) + 1
        res = collections.Counter(g["res"] for g in gs)
        best = max((g["best"] for g in gs if g["best"] is not None), default=None)
        wins = [g["ref"] for g in gs if g["res"] == "W"]
        rows.append(dict(name=name, col=col, n=len(gs), lines=len(lines), part=part, res=res, best=best, wins=wins,
                         top=lines.most_common(1)[0][1]))
    out = [f"# Cake census: {sum(r['n'] for r in rows)} games against Cake, {len(rows)} openings × colours", ""]
    out += ["## Openings where Cake varies most (distinct lines up to ply %d)" % a.plies, "",
            "| Opening | Regicide | games | Cake's distinct lines | most frequent line | lines first part at ply | W / D / L | best own score |",
            "|---|---|---|---|---|---|---|---|"]
    for r in sorted(rows, key=lambda r: (-r["lines"] / r["n"], -r["lines"]))[:40]:
        out.append(f"| {r['name']} | {r['col']} | {r['n']} | {r['lines']} | {r['top']} | {r['part'] or '—'} | "
                   f"{r['res']['W']} / {r['res']['D']} / {r['res']['L']} | {r['best']} |")
    out += ["", f"## Openings with a win of ours or our score above +{a.score}", "",
            "| Opening | Regicide | games | Cake's distinct lines | W / D / L | best own score | won games |",
            "|---|---|---|---|---|---|---|"]
    for r in sorted((r for r in rows if r["wins"] or (r["best"] is not None and r["best"] > a.score)),
                    key=lambda r: (-len(r["wins"]), -(r["best"] or 0))):
        out.append(f"| {r['name']} | {r['col']} | {r['n']} | {r['lines']} | {r['res']['W']} / {r['res']['D']} / "
                   f"{r['res']['L']} | {r['best']} | {', '.join(r['wins'][:4])}{' …' if len(r['wins']) > 4 else ''} |")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
