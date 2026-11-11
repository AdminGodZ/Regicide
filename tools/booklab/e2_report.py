#!/usr/bin/env python3
"""Experiment 2 read-out (lead 2026-11-11): Cake's play with kept and with fresh engine processes.

  e2_report.py --results DIR [--md OUT]
Runs: bl-e2a-fresh163, bl-e2b-kept163 (24 games of ACF #163 as White each), bl-e2c-{kept,fresh}-{1..6}-{h1,h2}
(the production deck). For every #163 game (all runs): worker and EngineGames (PDN tags of the runner), Cake's move,
depth and nodes at ply 15, 33 and 37 (from Cake's PDN comment), and the result. For the deck runs: per arm W / D / L
against Cake, wins, and the number of distinct Cake lines (both sides' moves from the end of the ballot to ply 20) per
opening and colour.
"""
import argparse
import collections
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of  # noqa: E402

B163 = None


def dn(c):
    d = re.search(r"depth (\d+)", c or "")
    n = re.search(r"nodes (\d+)", c or "")
    return f"d{d.group(1) if d else '?'}/{n.group(1) if n else '?'}"


def games(d):
    out = []
    for p in sorted(glob.glob(os.path.join(d, "games", "*.pdn"))):
        for tags, moves in parse_pdn(open(p, errors="replace").read()):
            rb = tags.get("Black", "").strip().lower().startswith("regicide")
            col = "B" if rb else "W"
            res = tags.get("Result", "").strip()
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
            out.append(dict(tags=tags, moves=moves, col=col, res=r, ballot=ballot_of(tags["Opening"]),
                            file=os.path.basename(p)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--md")
    a = ap.parse_args()
    R = a.results
    out = ["## ACF #163 as White: every game", "",
           "| Run | Game | Worker | EngineGames | Cake ply 15 | ply 33 | ply 37 | result | previous game of this worker's processes |",
           "|---|---|---|---|---|---|---|---|---|"]
    arms = collections.defaultdict(collections.Counter)
    runs = ["bl-e2a-fresh163", "bl-e2b-kept163"] + [f"bl-e2c-{arm}-{i}-{h}" for i in range(1, 7)
                                                    for arm in ("kept", "fresh") for h in ("h1", "h2")]
    lines = {"kept": collections.defaultdict(set), "fresh": collections.defaultdict(set)}
    deck = {"kept": collections.Counter(), "fresh": collections.Counter()}
    wins = {"kept": [], "fresh": []}
    for run in runs:
        d = os.path.join(R, run)
        if not os.path.isdir(d) or not os.path.exists(os.path.join(d, "exit_status")):
            continue  # only finished runs
        arm = "fresh" if "fresh" in run else "kept"
        gl = games(d)
        slot = {(g["tags"].get("Worker"), g["tags"].get("EngineGames")): g for g in gl}
        for g in gl:
            mv = g["moves"]
            if g["ballot"][0] == "ACF #163" and g["col"] == "W":
                cells = [f"{mv[i][0]} {dn(mv[i][1])}" if len(mv) > i else "—" for i in (14, 32, 36)]
                eg = g["tags"].get("EngineGames", "?")
                prev = slot.get((g["tags"].get("Worker"), str(int(eg) - 1))) if eg.isdigit() and int(eg) > 0 else None
                pv = (f"{prev['file']}: {prev['ballot'][0]} {prev['col']} {prev['res']}, {len(prev['moves'])} plies"
                      if prev else ("— (first game of the process)" if eg == "0" else "?"))
                out.append(f"| {run} | {g['file']} | {g['tags'].get('Worker', '?')} | {eg} | "
                           + " | ".join(cells) + f" | {g['res']} | {pv} |")
                arms[f"e2c-{arm}" if run.startswith('bl-e2c') else run][g["res"]] += 1
            if run.startswith("bl-e2c"):
                deck[arm][g["res"]] += 1
                key = (g["ballot"][0], g["col"])
                lines[arm][key].add(tuple(m for m, _ in mv[g["ballot"][2]:20]))
                if g["res"] == "W":
                    wins[arm].append(f"{g['ballot'][0]} {g['col']} ({run} {g['file']})")
    out += ["", "#163 as White, results per arm: " + "; ".join(f"{k}: {v['W']} / {v['D']} / {v['L']}" for k, v in arms.items()), ""]
    out += ["## Production deck against Cake: kept against fresh processes (6 decks each)", "",
            "| Arm | W / D / L | wins | openings × colours with more than one Cake line to ply 20 | mean distinct lines |",
            "|---|---|---|---|---|"]
    for arm in ("kept", "fresh"):
        c, ls = deck[arm], lines[arm]
        multi = sum(1 for v in ls.values() if len(v) > 1)
        mean = sum(len(v) for v in ls.values()) / len(ls) if ls else 0
        out.append(f"| {arm} | {c['W']} / {c['D']} / {c['L']} | {len(wins[arm])} | {multi} of {len(ls)} | {mean:.2f} |")
    out += ["", "Wins: " + "; ".join(f"{arm}: " + (", ".join(w) or "none") for arm, w in wins.items())]
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
