#!/usr/bin/env python3
"""WP9: conversion test of the missed database wins (WP8 table 1).

  wp9.py decks --scan-tsv dbscan_games.tsv --out DIR [--sample N --seed S] [--chunk 40]
      writes DIR/<opp>_<k>.txt deck files ("FEN |  | A=B": Regicide = engine A with the side that has the database win)
      and DIR/positions.tsv (id, opponent, colour, pieces, FEN, original game).
  wp9.py report --positions DIR/positions.tsv --scan booklab_dbscan --egdb DIR --out REPORT.md RESULTDIR...
      replays every result game, probes the database and reports converted / not converted per opponent and piece
      count, plies to the win, positions not converted (with the ending and the last 20 moves) and games where
      Regicide moved from a database-won position into a non-won one.
"""
import argparse
import collections
import csv
import os
import random
import statistics
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family  # noqa: E402

NAMES = {"cake": "Cake", "kingsrow": "KingsRow", "guinn": "GuiNN"}


def decks(a):
    rows = []
    for r in csv.DictReader(open(a.scan_tsv), delimiter="\t"):
        if r["winner"] not in ("B", "W"):
            continue
        wfam = r["bf"] if r["winner"] == "B" else r["wf"]
        if wfam != "regicide" or r["res"] == r["winner"]:
            continue
        opp = r["wf"] if r["winner"] == "B" else r["bf"]
        rows.append(dict(opp=opp, col=r["winner"], pieces=r["pieces"], fen=r["wfen"], src=f"{r['run']} {r['round']}"))
    if a.sample:
        random.Random(a.seed).shuffle(rows)
        rows = rows[:a.sample]
    os.makedirs(a.out, exist_ok=True)
    for i, r in enumerate(rows):
        r["id"] = f"p{i + 1}"
    by = collections.defaultdict(list)
    for r in rows:
        by[r["opp"]].append(r)
    for opp, rs in by.items():
        for k in range(0, len(rs), a.chunk):
            deck = f"{opp}_{k // a.chunk + 1}.txt"
            with open(os.path.join(a.out, deck), "w") as f:
                for j, r in enumerate(rs[k:k + a.chunk]):
                    f.write(f"{r['fen']} |  | A={r['col']}\n")
                    r["deck"], r["line"] = deck, str(j + 1)
    with open(os.path.join(a.out, "positions.tsv"), "w") as f:
        f.write("id\topp\tcolour\tpieces\tfen\tsource\tdeck\tline\n")
        for r in rows:
            f.write("\t".join([r["id"], r["opp"], r["col"], r["pieces"], r["fen"], r["src"], r["deck"], r["line"]]) + "\n")
    print({o: len(v) for o, v in by.items()}, "positions;", len(rows), "total")


def report(a):
    # games are mapped to positions by deck file and line ("Opening" tag: "/path/guinn_1.txt:3: ...")
    pos = {(r["deck"], r["line"]): r for r in csv.DictReader(open(a.positions), delimiter="\t")}

    def board(fen):  # side to move and piece sets, independent of token order
        parts = fen.strip().split(":")
        sets = {}
        for t in parts[1:]:
            t = t.strip()
            sets[t[:1].upper()] = frozenset(x.strip() for x in t[1:].split(",") if x.strip())
        return parts[0].strip().upper(), sets.get("W", frozenset()), sets.get("B", frozenset())

    games, seen, rejected = [], collections.Counter(), []
    for d in a.dirs:
        f = os.path.join(d, "games.pdn")
        if not os.path.exists(f):
            continue
        for tags, moves in parse_pdn(open(f, errors="replace").read()):
            parts = tags.get("Opening", "").split(":")
            key = (os.path.basename(parts[0]), parts[1].strip()) if len(parts) > 1 else None
            p = pos.get(key)
            if p is None:
                continue
            bf, wf = family(tags.get("Black", "")), family(tags.get("White", ""))
            col = "B" if bf == "regicide" else ("W" if wf == "regicide" else None)
            opp = wf if col == "B" else bf
            why = []
            if board(tags.get("FEN", "")) != board(p["fen"]):
                why.append(f"starting board {tags.get('FEN', '')} is not the planned {p['fen']}")
            if col != p["colour"]:
                why.append(f"Regicide plays {col}, planned {p['colour']}")
            if opp != p["opp"]:
                why.append(f"opponent {opp}, planned {p['opp']}")
            if why:
                rejected.append(f"{os.path.basename(d)} {tags.get('Round', '')} ({p['id']}): " + "; ".join(why))
                continue
            seen[p["id"]] += 1
            res = tags.get("Result", "")
            won = (res == "1-0" and col == "B") or (res == "0-1" and col == "W")
            games.append(dict(p=p, col=col, won=won, res=res, term=tags.get("Termination", ""), run=os.path.basename(d),
                              moves=[m for m, _ in moves]))
    missing = sorted((r["id"] for r in pos.values() if seen[r["id"]] == 0), key=lambda x: int(x[1:]))
    dup = sorted((i for i, n in seen.items() if n > 1), key=lambda x: int(x[1:]))
    coverage = (f"Coverage: {len(pos)} planned positions, {len(seen)} played, missing {len(missing)}"
                f"{' (' + ', '.join(missing) + ')' if missing else ''}, played more than once {len(dup)}"
                f"{' (' + ', '.join(dup) + ')' if dup else ''}; {len(rejected)} games rejected (board, colour or "
                f"opponent not as planned).")
    inp = "".join(f"{i}\t{g['p']['fen']}\t{' '.join(g['moves'])}\n" for i, g in enumerate(games))
    out = subprocess.run([a.scan, a.egdb, "1024"], input=inp, capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        f = line.split("\t")
        g = games[int(f[0])]
        if f[1] == "ERR":
            sys.exit(f"game {g['p']['id']} ({g['run']}): move {f[3]} at ply {f[2]} does not parse to one legal move")
        g.update(first=int(f[1]), winner=f[2], throw=int(f[5]), tmove=f[6], throws=int(f[8]), final=f[9],
                 quiet=int(f[12]) if len(f) > 12 else -1)
    md = [f"# WP9 conversion test: {len(games)} games from {len(pos)} positions", "", coverage, ""]
    for r in rejected:
        md.append(f"- rejected: {r}")
    if rejected:
        md.append("")
    conv = [g for g in games if g["won"]]
    md.append(f"**Converted: {len(conv)} of {len(games)} ({100 * len(conv) / max(1, len(games)):.1f}%).**")
    md.append("")
    for key, label in (("opp", "Opponent"), ("pieces", "Pieces at the start")):
        c = collections.defaultdict(lambda: [0, 0])
        for g in games:
            k = NAMES.get(g["p"][key], g["p"][key])
            c[k][0] += g["won"]
            c[k][1] += 1
        md.append(f"| {label} | Converted | Games |")
        md.append("|---|---|---|")
        for k, (w, n) in sorted(c.items()):
            md.append(f"| {k} | {w} | {n} |")
        md.append("")
    if conv:
        pl = [len(g["moves"]) for g in conv]
        q = statistics.quantiles(pl, n=4) if len(pl) > 1 else [pl[0]] * 3
        md.append(f"Plies to the win (converted games): median {statistics.median(pl):.0f}, quartiles {q[0]:.0f}–"
                  f"{q[2]:.0f}, max {max(pl)}.")
        qu = [g["quiet"] for g in conv]
        md.append(f"Highest no-progress count before the win (plies without a capture or man move; the 40-move rule "
                  f"draws at 79): median {statistics.median(qu):.0f}, max {max(qu)}; converted games that reached "
                  f">= 40: {sum(x >= 40 for x in qu)}, >= 60: {sum(x >= 60 for x in qu)}, >= 70: {sum(x >= 70 for x in qu)}.")
        md.append("")
    left = [g for g in games if g.get("throw", -1) >= 0 and g.get("winner") == g["col"]]
    md.append(f"**Games where Regicide moved from a database-won position into a non-won one: {len(left)}.**")
    for g in left:
        md.append(f"- {g['p']['id']} vs {NAMES[g['p']['opp']]}: ply {g['throw']} ({g['tmove']}, {g['throws']} such "
                  f"moves), result {g['res']} ({g['term']}); `{g['p']['fen']}`")
    md.append("")
    md.append("## Not converted")
    md.append("")
    md.append("| Id | Opponent | Regicide as | Pieces | Result | Termination | Max no-progress plies | FEN | Last 20 moves |")
    md.append("|---|---|---|---|---|---|---|---|---|")
    for g in games:
        if not g["won"]:
            md.append(f"| {g['p']['id']} | {NAMES[g['p']['opp']]} | {g['col']} | {g['p']['pieces']} | {g['res']} | "
                      f"{g['term']} | {g['quiet']} | `{g['p']['fen']}` | {' '.join(g['moves'][-20:])} |")
    md.append("")
    md.append("## Every game")
    md.append("")
    md.append("| Id | Opponent | Regicide as | Pieces | Result | Plies | Max no-progress plies | Left the won region |")
    md.append("|---|---|---|---|---|---|---|---|")
    for g in sorted(games, key=lambda g: int(g["p"]["id"][1:])):
        md.append(f"| {g['p']['id']} | {NAMES[g['p']['opp']]} | {g['col']} | {g['p']['pieces']} | "
                  f"{'won' if g['won'] else g['res']} | {len(g['moves'])} | {g['quiet']} | "
                  f"{'ply ' + str(g['throw']) if g.get('throw', -1) >= 0 and g.get('winner') == g['col'] else '-'} |")
    open(a.out, "w").write("\n".join(md) + "\n")
    print("\n".join(md[:20]))


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    d = sub.add_parser("decks")
    d.add_argument("--scan-tsv", required=True)
    d.add_argument("--out", required=True)
    d.add_argument("--sample", type=int, default=0)
    d.add_argument("--seed", type=int, default=9)
    d.add_argument("--chunk", type=int, default=40)
    r = sub.add_parser("report")
    r.add_argument("--positions", required=True)
    r.add_argument("--scan", required=True)
    r.add_argument("--egdb", required=True)
    r.add_argument("--out", required=True)
    r.add_argument("dirs", nargs="+")
    a = ap.parse_args()
    {"decks": decks, "report": report}[a.cmd](a)


if __name__ == "__main__":
    main()
