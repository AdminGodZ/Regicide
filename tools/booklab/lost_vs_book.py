#!/usr/bin/env python3
"""For lost games: the own move where the game left the non-lost games of the same opening and colour, and what a
book plays there (and before).

  lost_vs_book.py --book book.bin --pins pins.txt --decisions decisions.txt --pinwalk booklab_pinwalk --egdb DIR
                  --lost RUN:ROUND ... --pool RUNDIR ...
--pool: result directories whose non-lost games (exactly one side named Regicide) are the reference.
For each lost game, walking Regicide's positions from the end of the ballot: the leaving move is the first position
where non-lost pool games of the same opening and colour reached the same position and none of them played the lost
game's move. Reported: that ply, the moves the pool played there, the book's move there (pin, decision or none), how
many of the lost game's own positions before it are pinned, and the first position where the book plays a move
different from the lost game's (the book leaves the lost line there).
"""
import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of  # noqa: E402


def games_of(path):
    out = []
    for tags, moves in parse_pdn(open(path, errors="replace").read()):
        rb = tags.get("Black", "").strip().lower().startswith("regicide")
        rw = tags.get("White", "").strip().lower().startswith("regicide")
        if rb == rw:
            continue
        col = "B" if rb else "W"
        res = tags.get("Result", "").strip()
        r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
        out.append(dict(tags=tags, col=col, res=r, moves=[m for m, _ in moves], ballot=ballot_of(tags["Opening"])))
    return out


def walk(pinwalk, book, egdb, gs):
    lines = "".join(f"{i}\t{g['tags'].get('FEN', '')}\t{g['col']}\t{g['ballot'][2]}\t{' '.join(g['moves'])}\n"
                    for i, g in enumerate(gs))
    out = subprocess.run([pinwalk, book, egdb], input=lines, capture_output=True, text=True, check=True).stdout
    per = collections.defaultdict(list)
    for l in out.splitlines():
        f = l.split("\t")
        if f[1] == "ERR":
            continue
        per[int(f[0])].append((int(f[1]), f[2], f[3], f[4] == "1", f[5]))
    return per


def main():
    ap = argparse.ArgumentParser()
    for k in ("book", "pins", "decisions", "pinwalk", "egdb"):
        ap.add_argument("--" + k, required=True)
    ap.add_argument("--lost", nargs="+", required=True)
    ap.add_argument("--pool", nargs="+", required=True)
    a = ap.parse_args()
    pins = {l.split(" | ")[0] for l in open(a.pins) if " | " in l and not l.startswith("#")}
    decs = {l.split(" | ")[0] for l in open(a.decisions) if " | " in l and not l.startswith("#")} - pins
    pool = [g for d in a.pool for g in games_of(os.path.join(d, "games.pdn")) if g["res"] in ("W", "D")]
    pw = walk(a.pinwalk, a.book, a.egdb, pool)
    seen = collections.defaultdict(lambda: collections.defaultdict(set))  # (opening, col) -> fen -> {moves}
    for i, g in enumerate(pool):
        for ply, fen, mv, el, bm in pw[i]:
            seen[(g["ballot"][1], g["col"])][fen].add(mv)
    print("| Lost game | Regicide | leaving move (ply: lost / non-lost games) | book at that position | own positions before it pinned | first position where the book leaves the lost line |")
    print("|---|---|---|---|---|---|")
    for spec in a.lost:
        run, rnd = spec.rsplit(":", 1)
        g = next(x for x in games_of(os.path.join(run, "games.pdn")) if x["tags"]["Round"] == rnd)
        rows = walk(a.pinwalk, a.book, a.egdb, [g])[0]
        ref = seen[(g["ballot"][1], g["col"])]
        leave = next(((ply, fen, mv) for ply, fen, mv, el, bm in rows if fen in ref and mv not in ref[fen]), None)
        before = [r for r in rows if leave is None or r[0] < leave[0]]
        npin = sum(1 for r in before if r[1] in pins)
        first = next((r for r in rows if r[3] and r[4] != "-" and r[4] != r[2]), None)
        src = lambda fen: "pin" if fen in pins else ("decision" if fen in decs else "old book")
        if leave:
            bm = next(r[4] for r in rows if r[0] == leave[0])
            book_there = f"{bm} ({src(leave[1])})" if bm != "-" else "none"
            lv = f"ply {leave[0] + 1}: {leave[2]} / {', '.join(sorted(ref[leave[1]]))}"
        else:
            book_there, lv = "-", "none (no non-lost game reached a position where it differed)"
        fr = f"ply {first[0] + 1}: {first[4]} instead of {first[2]} ({src(first[1])})" if first else "none"
        print(f"| {g['ballot'][0]} ({os.path.basename(run)} {rnd}) | {g['col']} | {lv} | {book_there} | {npin} of {len(before)} | {fr} |")


if __name__ == "__main__":
    main()
