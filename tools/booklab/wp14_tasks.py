#!/usr/bin/env python3
"""WP14 step 1 (lead 2026-11-11 13:30): rootscore tasks for the near-miss games.

  wp14_tasks.py --table near-miss-games.tsv --results DIR --skip "ACF #6 Inferno" --out tasks.tsv --contexts ctx.tsv
For every game of the reviewer's near-miss table (Regicide Black, drawn, own search fell to -56 .. -136), except the
skipped openings: every own position from the end of the ballot up to the game's lowest-score ply, as a rootscore
task (start position, the moves before it, and the move played to score alone). Positions shared by several games
(the same move list) are one task. ctx.tsv lists the contexts: id, opponent, opening, run, round, lowest score and
ply, first searched ply (book end), and the task ids of the game in order.
"""
import argparse
import csv
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--table", required=True)
    ap.add_argument("--results", required=True)
    ap.add_argument("--skip", nargs="*", default=[])
    ap.add_argument("--out", required=True)
    ap.add_argument("--contexts", required=True)
    a = ap.parse_args()
    rows = list(csv.DictReader(open(a.table), delimiter="\t"))
    tasks, ctx = {}, []
    for r in sorted(rows, key=lambda r: int(r["lowest_score"])):
        if r["opening"] in a.skip:
            continue
        g = next((m for t, m in parse_pdn(open(os.path.join(a.results, "bl-" + r["run"], "games.pdn"), errors="replace").read())
                  if t["Round"] == r["round"]), None)
        if g is None:
            sys.exit(f"{r['run']} {r['round']} not found")
        mv = [x for x, _ in g]
        nopen = len(ballot_of(next(t for t, _ in parse_pdn(open(os.path.join(a.results, "bl-" + r["run"], "games.pdn"),
                                                                           errors="replace").read())
                                    if t["Round"] == r["round"])["Opening"])[1].split())
        ids = []
        for p in range(nopen + 1, int(r["lowest_score_ply"]) + 1):
            if p % 2 == 0:  # Regicide is Black: own moves at odd plies
                continue
            key = " ".join(mv[:p - 1])
            tid = tasks.get(key, (f"T{len(tasks) + 1:03d}", None))[0]
            tasks[key] = (tid, mv[p - 1])
            kind = "book" if p < int(r["first_searched_ply"]) else "search"
            ids.append(f"{tid}:{p}:{mv[p - 1]}:{kind}")
        cid = f"{r['opponent']} {r['opening']} ({r['run']} {r['round']})"
        ctx.append([cid, r["opponent"], r["opening"], r["run"], r["round"], r["lowest_score"], r["lowest_score_ply"],
                    r["first_searched_ply"], " ".join(ids)])
    with open(a.out, "w") as fo:
        for key, (tid, played) in tasks.items():
            fo.write(f"{tid}\t\t{key}\t{played}\n")
    with open(a.contexts, "w") as fo:
        fo.write("context\topponent\topening\trun\tround\tlowest_score\tlowest_ply\tfirst_searched_ply\ttasks\n")
        for c in ctx:
            fo.write("\t".join(c) + "\n")
    print(f"{len(ctx)} contexts, {len(tasks)} positions")


if __name__ == "__main__":
    main()
