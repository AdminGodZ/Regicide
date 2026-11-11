#!/usr/bin/env python3
"""Validation read-out (lead 2026-11-11): per run W/D/L; every loss with the leaving-move table; every win against
Cake and KingsRow with the opponent's first move that differs from its drawn games; games identical move for move to
an earlier game of the same pairing.

  val_report.py --results DIR --prefix v11b --nets n6c n8q --runs "kingsrow a" "kingsrow b" "cake a" "cake b" "guinn a"
                [--pool RUNDIR ...]
Runs are DIR/bl-<prefix>-<net>-<opp>-<run>-h1/-h2. A pairing is (net, opponent); "earlier" means an earlier run of
the same pairing in the order given (a before b), compared by opening and colour.
Losses: the own move where the lost game left the non-lost games of its opening and colour (pool: --pool runs plus all
runs of this validation), and the first own score below -100 that is not at a capture.
Wins: the opponent's first move, from the end of the ballot, that no drawn game of the same opening and colour
against the same opponent (pool and this validation) played in that position.
"""
import argparse
import collections
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of, family  # noqa: E402

REPLAY = os.path.expanduser("~/regicide-booklab/build-linux/booklab_replay")


def load(path):
    out = []
    for tags, moves in parse_pdn(open(path, errors="replace").read()):
        rb = tags.get("Black", "").strip().lower().startswith("regicide")
        rw = tags.get("White", "").strip().lower().startswith("regicide")
        if rb == rw:
            continue
        col = "B" if rb else "W"
        res = tags.get("Result", "").strip()
        r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
        b = ballot_of(tags["Opening"])
        out.append(dict(col=col, res=r, moves=[m for m, _ in moves], comments=[c for _, c in moves], ballot=b,
                        opp=family(tags["White"] if rb else tags["Black"]), rnd=tags.get("Round", "?"),
                        term=tags.get("Termination", ""), src=path))
    return out


def keys_of(gs):
    """position keys before every ply of every game (one replay call)"""
    inp = "".join(f"{i}\t\t{' '.join(g['moves'])}\n" for i, g in enumerate(gs))
    out = subprocess.run([REPLAY], input=inp, capture_output=True, text=True, check=True).stdout
    keys = collections.defaultdict(dict)
    for l in out.splitlines():
        f = l.split("\t")
        if f[0] == "P":
            keys[int(f[1])][int(f[2])] = f[3]
    return keys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--prefix", required=True)
    ap.add_argument("--nets", nargs="+", required=True)
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--pool", nargs="*", default=[])
    ap.add_argument("--md")
    a = ap.parse_args()
    runs = []  # (net, opp, letter, games)
    for net in a.nets:
        for spec in a.runs:
            opp, letter = spec.split()
            gs = []
            for h in ("h1", "h2"):
                p = os.path.join(a.results, f"bl-{a.prefix}-{net}-{opp}-{letter}-{h}", "games.pdn")
                if os.path.exists(p):
                    gs += load(p)
            runs.append((net, opp, letter, gs))
    pool = [g for d in a.pool for g in load(os.path.join(d, "games.pdn"))]
    allg = pool + [g for r in runs for g in r[3]]
    ks = keys_of(allg)
    for i, g in enumerate(allg):
        g["keys"] = ks[i]
    # reference sets: (opening, colour) -> key -> moves of Regicide in non-lost games; (opening, colour, opp) -> key ->
    # moves of the opponent in drawn games
    ours, theirs = collections.defaultdict(lambda: collections.defaultdict(set)), \
        collections.defaultdict(lambda: collections.defaultdict(set))
    for g in allg:
        oc = (g["ballot"][1], g["col"])
        for i, mv in enumerate(g["moves"]):
            if i < g["ballot"][2] or i not in g["keys"]:
                continue
            own = (i % 2 == 0) == (g["col"] == "B")
            if own and g["res"] in ("W", "D"):
                ours[oc][g["keys"][i]].add(mv)
            if not own and g["res"] == "D":
                theirs[oc + (g["opp"],)][g["keys"][i]].add(mv)
    out = ["| Net | Opponent | Run | Games | W / D / L | identical to a game of an earlier run of the pairing |",
           "|---|---|---|---|---|---|"]
    earlier = collections.defaultdict(dict)  # (net, opp) -> (opening, colour) -> [move lists]
    losses, wins = [], []
    # earlier won games (pool first, then the runs in order) for "repeats an earlier won game move for move"
    won_before = collections.defaultdict(list)  # (opening, colour, opp) -> [(moves, ref)]
    for g in pool:
        if g["res"] == "W":
            won_before[(g["ballot"][1], g["col"], g["opp"])].append(
                (g["moves"], f"{os.path.basename(os.path.dirname(g['src']))} {g['rnd']}"))
    for net, opp, letter, gs in runs:
        c = collections.Counter(g["res"] for g in gs)
        same = sum(1 for g in gs if g["moves"] in earlier[(net, opp)].get((g["ballot"][1], g["col"]), []))
        out.append(f"| {net} | {opp} | {letter} | {len(gs)} | {c['W']} / {c['D']} / {c['L']}"
                   + (f" ({c['?']} unfinished)" if c["?"] else "") + f" | {same if earlier[(net, opp)] else '— (first run)'} |")
        for g in gs:
            earlier[(net, opp)].setdefault((g["ballot"][1], g["col"]), []).append(g["moves"])
            oc = (g["ballot"][1], g["col"])
            tag = f"{os.path.basename(os.path.dirname(g['src']))} {g['rnd']}"
            if g["res"] == "L":
                leave = next((f"ply {i + 1}: {mv} / {', '.join(sorted(ours[oc][g['keys'][i]]))}" for i, mv in enumerate(g["moves"])
                              if i >= g["ballot"][2] and (i % 2 == 0) == (g["col"] == "B") and i in g["keys"]
                              and ours[oc].get(g["keys"][i]) and mv not in ours[oc][g["keys"][i]]),
                             "none (each of our moves was also played in non-lost games)")
                drop = next((i + 1 for i, cm in enumerate(g["comments"]) if (i % 2 == 0) == (g["col"] == "B")
                             and "x" not in g["moves"][i] and (s := re.search(r"\bscore (-?\d+)", cm or ""))
                             and int(s.group(1)) < -100), None)
                losses.append(f"| {net} | {g['ballot'][0]} | {g['col']} | {tag} | {leave} | {drop or '—'} | {len(g['moves'])} |")
            if g["res"] == "W" and opp in ("cake", "kingsrow"):
                ref = theirs[oc + (opp,)]
                dev = next((f"ply {i + 1}: {mv} (drawn games: {', '.join(sorted(ref[g['keys'][i]]))})" for i, mv in enumerate(g["moves"])
                            if i >= g["ballot"][2] and (i % 2 == 0) != (g["col"] == "B") and i in g["keys"]
                            and ref.get(g["keys"][i]) and mv not in ref[g["keys"][i]]),
                           "none (each of its moves was also played in drawn games)")
                prev = won_before[oc + (opp,)]
                rep = next((r for m, r in prev if m == g["moves"]), None)
                if rep:
                    repeat = f"yes, move for move: {rep}"
                elif prev:
                    p = max(prev, key=lambda mr: next((i for i in range(min(len(mr[0]), len(g["moves"])))
                                                       if mr[0][i] != g["moves"][i]), min(len(mr[0]), len(g["moves"]))))
                    k = next((i for i in range(min(len(p[0]), len(g["moves"]))) if p[0][i] != g["moves"][i]), None)
                    repeat = f"no; closest earlier win {p[1]}, first difference ply {k + 1 if k is not None else '—'}"
                else:
                    repeat = "no earlier win of this opening and colour"
                wins.append(f"| {net} | {opp} | {g['ballot'][0]} | {g['col']} | {tag} | {dev} | {repeat} |")
                won_before[oc + (opp,)].append((g["moves"], tag))
    out += ["", "Losses (leaving move: the lost game's own move / the moves of the non-lost games in that position):", "",
            "| Net | Opening | Regicide | Game | leaving move | first own score < −100 (not at a capture) | plies |",
            "|---|---|---|---|---|---|---|"] + (losses or ["| — | | | | | | |"])
    out += ["", "Wins against Cake and KingsRow (the opponent's first move not played by it in drawn games of the same "
            "opening and colour; and whether the game repeats an earlier won game, of the pool or of an earlier run, move "
            "for move):", "", "| Net | Opponent | Opening | Regicide | Game | opponent's first different move | repeats an earlier win? |",
            "|---|---|---|---|---|---|---|"] + (wins or ["| — | | | | | | |"])
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
