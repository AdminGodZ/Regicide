#!/usr/bin/env python3
"""Won games against KingsRow and Cake: does a book follow our moves of the game at every own position?

  wins_vs_book.py --book book.bin --pins pins.txt --decisions decisions.txt --pinwalk booklab_pinwalk --egdb DIR
                  --runs RUNDIR ... [--opps kingsrow cake]
For each game Regicide won against one of --opps in --runs: walk our positions (end of the ballot to 8 pieces) with
the book; the first eligible position where the book plays another move than the game did, or avoids the game's move,
is where the book leaves the won game (pin, decision or old book). There the evidence of both moves: every game of
--runs through that position with Regicide to move, per move: number, results, opponents.
"""
import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of, family  # noqa: E402


def load(d):
    out = []
    for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
        rb = tags.get("Black", "").strip().lower().startswith("regicide")
        rw = tags.get("White", "").strip().lower().startswith("regicide")
        if rb == rw:
            continue
        col = "B" if rb else "W"
        res = tags.get("Result", "").strip()
        r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
        out.append(dict(col=col, res=r, moves=[m for m, _ in moves], ballot=ballot_of(tags["Opening"]),
                        opp=family(tags["White"] if rb else tags["Black"]), run=os.path.basename(d.rstrip("/")),
                        rnd=tags.get("Round", "?"), fen=tags.get("FEN", "")))
    return out


def main():
    ap = argparse.ArgumentParser()
    for k in ("book", "pins", "decisions", "pinwalk", "egdb"):
        ap.add_argument("--" + k, required=True)
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--opps", nargs="+", default=["kingsrow", "cake"])
    ap.add_argument("--md")
    a = ap.parse_args()
    pins = {l.split(" | ")[0] for l in open(a.pins) if " | " in l and not l.startswith("#")}
    decs = {l.split(" | ")[0] for l in open(a.decisions) if " | " in l and not l.startswith("#")} - pins
    gs = [g for d in sorted(a.runs) for g in load(d)]
    inp = "".join(f"{i}\t{g['fen']}\t{g['col']}\t{g['ballot'][2]}\t{' '.join(g['moves'])}\n" for i, g in enumerate(gs))
    out = subprocess.run([a.pinwalk, a.book, a.egdb], input=inp, capture_output=True, text=True, check=True).stdout
    walk = collections.defaultdict(list)
    at = collections.defaultdict(lambda: collections.defaultdict(list))  # fen -> move -> [game]
    errors = {}  # review 2026-11-11 finding 5: a game whose replay fails is reported, never counted as followed
    for l in out.splitlines():
        f = l.split("\t")
        if f[1] == "ERR":
            errors[int(f[0])] = " ".join(f[2:])
            continue
        gi, ply, fen, mv, el, bm = int(f[0]), int(f[1]), f[2], f[3], f[4] == "1", f[5]
        av = set(f[6].split(",")) if len(f) > 6 and f[6] != "-" else set()
        walk[gi].append((ply, fen, mv, el, bm, av))
        at[fen][mv].append(gi)
    src = lambda fen: "pin" if fen in pins else ("decision" if fen in decs else "old book")
    def ev(fen, mv):
        lst = at[fen][mv]
        res = collections.Counter(gs[i]["res"] for i in lst)
        opp = collections.Counter(gs[i]["opp"] for i in lst)
        return f"{mv}: {len(lst)} games, {dict(res)}, {dict(opp)}"
    rows = ["| Won game | Opponent | Regicide | own positions walked | no forced departure? (the book never forces another "
            "move nor avoids ours) | where the book forces a departure | first consulted position without a book entry | "
            "evidence (won move; book move) |",
            "|---|---|---|---|---|---|---|---|"]
    for gi, g in enumerate(gs):
        if g["res"] != "W" or g["opp"] not in a.opps:
            continue
        w = walk[gi]
        if gi in errors:
            rows.append(f"| {g['ballot'][0]} ({g['run']} {g['rnd']}) | {g['opp']} | {g['col']} | — | REPLAY FAILED at "
                        f"{errors[gi]} | — | — | — |")
            continue
        # forced departure: the book forces another move where it is consulted, or avoids our move (the avoid list
        # applies at every reported root, also inside a run of king moves; review 2026-11-11 residual)
        leave = next((r for r in w if (r[3] and r[4] != "-" and r[4] != r[2]) or r[2] in r[5]), None)
        # first own position where the book is consulted but has no entry (from there the engine searches)
        nobook = next((r for r in w if r[3] and r[4] == "-" and r[2] not in r[5]), None)
        nb = f"ply {nobook[0] + 1}" if nobook else "none (book entry at every consulted position)"
        name = f"{g['ballot'][0]} ({g['run']} {g['rnd']})"
        if leave is None:
            rows.append(f"| {name} | {g['opp']} | {g['col']} | {len(w)} | yes | — | {nb} | — |")
        else:
            ply, fen, mv, el, bm, av = leave
            bmove = bm if bm != "-" else "(won move avoided)"
            rows.append(f"| {name} | {g['opp']} | {g['col']} | {len(w)} | no | ply {ply + 1}: {bmove} instead of {mv} "
                        f"({src(fen)}) | {nb} | {ev(fen, mv)}; {ev(fen, bm) if bm != '-' else '—'} |")
    if errors:
        rows += ["", f"**Replay errors: {len(errors)} games could not be replayed:** " + "; ".join(
            f"{gs[i]['run']} {gs[i]['rnd']} ({e})" for i, e in sorted(errors.items()))]
    text = "\n".join(rows)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")
    if errors:
        sys.exit(3)


if __name__ == "__main__":
    main()
