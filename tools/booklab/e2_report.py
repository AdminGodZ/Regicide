#!/usr/bin/env python3
"""Experiment 2 read-out (lead 2026-11-11): the opponent's play with kept and with fresh engine processes.

  e2_report.py --results DIR --opening "ACF #163" --colour W --plies 15 33 37 --runs RUN ... [--deck-runs RUN ...]
               [--md OUT] [--title TEXT]
For every game of OPENING with Regicide playing COLOUR in RUNS (only finished runs, i.e. with exit_status): worker,
the opponent process's game counter (PDN tag EngineGamesB of master c61b030 and later, or the single EngineGames of
ddd25fc; the opponent is engine B in all our runs), the opponent's move, depth and nodes at each named ply (from its
PDN comment), the result, and the previous game of the opponent's process (same worker, counter - 1). The runner
that wrote each run is reported (tag form). With --deck-runs: per arm (kept / fresh, from the run name) W / D / L,
wins, and distinct opponent lines (both sides' moves from the end of the ballot to ply 20) per opening and colour.
"""
import argparse
import collections
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of  # noqa: E402


def dn(c):
    """depth/nodes from an engine's PDN comment. Cake and Regicide write 'nodes N'; KingsRow (book mode 3) writes
    'value=V, depth D/..., T s, R kN/s' without nodes: then value and ~R x T thousand nodes."""
    c = c or ""
    d = re.search(r"depth (\d+)", c)
    n = re.search(r"nodes (\d+)", c)
    if "book" in c and not d:
        return "book"
    if not n:
        v = re.search(r"value=(-?\d+)", c)
        k = re.search(r", ([\d.]+)s, (\d+) kN/s", c)
        nn = f"~{float(k.group(1)) * int(k.group(2)):.0f}k" if k else "?"
        return f"d{d.group(1) if d else '?'}/{nn}" + (f" v{v.group(1)}" if v else "")
    return f"d{d.group(1) if d else '?'}/{n.group(1)}"


def counters(tags):
    """(opponent process counter, Regicide process counter, tag form)"""
    if "EngineGamesB" in tags:
        return tags["EngineGamesB"], tags.get("EngineGamesA", "?"), "A/B"
    return tags.get("EngineGames", "?"), tags.get("EngineGames", "?"), "single"


def games(d):
    out = []
    for p in sorted(glob.glob(os.path.join(d, "games", "*.pdn"))):
        for tags, moves in parse_pdn(open(p, errors="replace").read()):
            rb = tags.get("Black", "").strip().lower().startswith("regicide")
            col = "B" if rb else "W"
            res = tags.get("Result", "").strip()
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
            out.append(dict(tags=tags, moves=moves, col=col, res=r, ballot=ballot_of(tags["Opening"]),
                            file=os.path.basename(p), eg=counters(tags)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--opening", required=True)
    ap.add_argument("--colour", required=True)
    ap.add_argument("--plies", type=int, nargs="+", required=True)
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--deck-runs", nargs="*", default=[])
    ap.add_argument("--title")
    ap.add_argument("--md")
    a = ap.parse_args()
    R = a.results
    out = [f"## {a.title or a.opening + ' as ' + a.colour}: every game", "",
           "| Run | Game | Worker | opponent's game no. (Regicide's) | " + " | ".join(f"ply {p}" for p in a.plies)
           + " | result | previous game of the opponent's process |", "|---|---|---|---|" + "---|" * len(a.plies) + "---|---|"]
    per_run = collections.defaultdict(collections.Counter)
    by_eg = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))  # run -> counter -> move@first ply
    forms = {}
    for run in a.runs:
        d = os.path.join(R, run)
        if not os.path.exists(os.path.join(d, "exit_status")):
            continue
        gl = games(d)
        slot = {(g["tags"].get("Worker"), g["eg"][0]): g for g in gl}
        for g in gl:
            if g["ballot"][0] != a.opening or g["col"] != a.colour:
                continue
            forms[run] = g["eg"][2]
            mv = g["moves"]
            cells = [f"{mv[p - 1][0]} {dn(mv[p - 1][1])}" if len(mv) >= p else "—" for p in a.plies]
            eg = g["eg"][0]
            prev = slot.get((g["tags"].get("Worker"), str(int(eg) - 1))) if eg.isdigit() and int(eg) > 0 else None
            pv = (f"{prev['file']}: {prev['ballot'][0]} Regicide {prev['col']}, {prev['res']}, {len(prev['moves'])} plies"
                  if prev else ("— (first game of the process)" if eg == "0" else "?"))
            out.append(f"| {run} | {g['file']} | {g['tags'].get('Worker', '?')} | {eg} ({g['eg'][1]}) | "
                       + " | ".join(cells) + f" | {g['res']} | {pv} |")
            per_run[run][g["res"]] += 1
            by_eg[run][eg][(mv[a.plies[0] - 1][0] if len(mv) >= a.plies[0] else "—", g["res"])] += 1
    out += ["", "| Run | W / D / L | by the opponent process's game number: (move at ply %d, result) × games |" % a.plies[0],
            "|---|---|---|"]
    for run, c in per_run.items():
        egs = "; ".join(f"{eg}: " + ", ".join(f"{m} {r} ×{n}" for (m, r), n in sorted(v.items()))
                        for eg, v in sorted(by_eg[run].items(), key=lambda kv: int(kv[0]) if kv[0].isdigit() else 99))
        out.append(f"| {run} | {c['W']} / {c['D']} / {c['L']} | {egs} |")
    out += ["", "Runner tag form per run: " + ", ".join(f"{r}: {f}" for r, f in forms.items())]
    if a.deck_runs:
        lines = {"kept": collections.defaultdict(set), "fresh": collections.defaultdict(set)}
        deck = {"kept": collections.Counter(), "fresh": collections.Counter()}
        wins = {"kept": [], "fresh": []}
        nruns = collections.Counter()
        for run in a.deck_runs:
            d = os.path.join(R, run)
            if not os.path.exists(os.path.join(d, "exit_status")):
                continue
            arm = "fresh" if "fresh" in run else "kept"
            nruns[arm] += 1
            for g in games(d):
                deck[arm][g["res"]] += 1
                lines[arm][(g["ballot"][0], g["col"])].add(tuple(m for m, _ in g["moves"][g["ballot"][2]:20]))
                if g["res"] == "W":
                    wins[arm].append(f"{g['ballot'][0]} {g['col']} ({run} {g['file']})")
        out += ["", "## Production deck: kept against fresh processes", "",
                "| Arm | half-runs | W / D / L | wins | openings × colours with more than one line to ply 20 | mean distinct lines |",
                "|---|---|---|---|---|---|"]
        for arm in ("kept", "fresh"):
            c, ls = deck[arm], lines[arm]
            multi = sum(1 for v in ls.values() if len(v) > 1)
            mean = sum(len(v) for v in ls.values()) / len(ls) if ls else 0
            out.append(f"| {arm} | {nruns[arm]} | {c['W']} / {c['D']} / {c['L']} | {len(wins[arm])} | {multi} of {len(ls)} | {mean:.2f} |")
        out += ["", "Wins: " + "; ".join(f"{arm}: " + (", ".join(w) or "none") for arm, w in wins.items())]
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
