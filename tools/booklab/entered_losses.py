#!/usr/bin/env python3
"""Database-lost positions Regicide entered in the WP9 step-4 runs (lead's question (a) vs (b)).

  entered_losses.py --scan-tsv DIR/dbscan_games.tsv --results ~/regicide-match/results --replay booklab_replay
                    --out report.md --list entered_losses.txt [--build conv3 --other m11e]

Input: a dbscan.py scan of the step-4 runs made with --quiet --no-dedup (a database win counts only when the side to
move has no capture pending; identical games of the two builds are kept), with per-side first-win plies.
Output:
  1. per build and opponent: games where the opponent / Regicide had a database win, and how many were converted;
  2. for every game where the opponent had a database win against --build: ballot, colour, ply and pieces of the first
     lost position, Regicide's own PDN score on its move before it, the first Regicide move scored <= -100 (ply,
     pieces), the result, and whether --other played the same game up to that ply;
  3. --list: the first lost position and the position four plies earlier, per game.
"""
import argparse
import collections
import csv
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn  # noqa: E402

NAMES = {"cake": "Cake", "kingsrow": "KingsRow", "guinn": "GuiNN"}
SCORE = re.compile(r"\bscore\s+(-?\d+)")


def pieces(fen):
    return sum(len([x for x in part[1:].split(",") if x.strip()]) for part in fen.split(":")[1:])


def fens_along(replay, moves):
    """FEN before each ply (index = ply) by replaying the whole game in a fresh process."""
    out = subprocess.run([replay], input=f"1\t\t{' '.join(moves)}\n", capture_output=True, text=True, check=True).stdout
    fen_of, key_at, final = {}, {}, None
    for line in out.splitlines():
        p = line.split("\t")
        if p[0] == "F":
            fen_of[p[1]] = p[2]
        elif p[0] == "P":
            key_at[int(p[2])] = p[3]
        elif p[0] == "Z":
            final = p[3]
    return [fen_of[key_at[i]] for i in range(len(key_at))] + [final]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scan-tsv", required=True)
    ap.add_argument("--results", required=True)
    ap.add_argument("--replay", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--list", required=True)
    ap.add_argument("--build", default="conv3")
    ap.add_argument("--other", default="m11e")
    a = ap.parse_args()
    rows = list(csv.DictReader(open(a.scan_tsv), delimiter="\t"))
    pdn_cache = {}

    def game(run, rnd):
        if run not in pdn_cache:
            f = os.path.join(a.results, run, "games.pdn")
            pdn_cache[run] = {t.get("Round"): m for t, m in parse_pdn(open(f, errors="replace").read())}
        return pdn_cache[run].get(rnd)

    t = collections.defaultdict(collections.Counter)
    detail = []
    for r in rows:
        p = r["run"].split("-")  # bl-s4-conv3-cake-h1
        build, opp = p[2], p[3]
        rc = "B" if r["bf"] == "regicide" else "W"
        oc = "W" if rc == "B" else "B"
        of = int(r["first_" + oc.lower()])
        rf = int(r["first_" + rc.lower()])
        k = (build, opp)
        t[k]["games"] += 1
        if of >= 0:
            t[k]["opp_reached"] += 1
            t[k]["opp_converted"] += r["res"] == oc
        if rf >= 0:
            t[k]["reg_reached"] += 1
            t[k]["reg_converted"] += r["res"] == rc
        if build == a.build and of >= 0:
            detail.append((r, opp, rc, of))
    md = ["# Database-lost positions entered by Regicide (WP9 step 4)", "",
          "Scan of the step-4 runs with `dbscan.py --quiet --no-dedup`: a database win is counted only in a position "
          "where the side to move has no capture pending; identical games of the two builds are kept. Per side, the "
          "first won position.", "",
          "| Build | Opponent | Games | Opponent had a db win | it converted | Regicide had a db win | it converted |",
          "|---|---|---|---|---|---|---|"]
    for (b, o), c in sorted(t.items()):
        md.append(f"| {b} | {NAMES[o]} | {c['games']} | {c['opp_reached']} | {c['opp_converted']} | {c['reg_reached']} | "
                  f"{c['reg_converted']} |")
    md += ["", f"## Every game in which the opponent had a database win against {a.build}", "",
           "| Run | Round | Ballot | Regicide as | First lost ply | Pieces | Regicide's score on its move before | "
           f"First Regicide score <= -100 (ply, pieces) | Result | {a.other} up to that ply |",
           "|---|---|---|---|---|---|---|---|---|---|"]
    lst = ["# Database-lost positions Regicide entered (WP9 step 4, build " + a.build + "): first quiet lost position "
           "(opponent to move or Regicide to move) and the position four plies earlier.",
           "# FEN lost | FEN four plies earlier | run round ballot, Regicide colour, opponent, result"]
    for r, opp, rc, of in sorted(detail, key=lambda x: (x[0]["run"], x[0]["round"])):
        mv = game(r["run"], r["round"])
        moves = [m for m, _ in mv]
        fens = fens_along(a.replay, moves)
        regp = [i for i in range(of) if (i % 2 == 0) == (rc == "B")]
        before = regp[-1] if regp else None
        sc_before = None
        if before is not None and mv[before][1]:
            m = SCORE.search(mv[before][1])
            sc_before = int(m.group(1)) if m else None
        first_neg = None
        for i in regp:
            c = mv[i][1] or ""
            m = SCORE.search(c)
            if m and int(m.group(1)) <= -100:
                first_neg = (i, pieces(fens[i]))
                break
        other_run = r["run"].replace(f"-{a.build}-", f"-{a.other}-")
        om = game(other_run, r["round"])
        if om is None:
            same = "not played"
        else:
            omoves = [m for m, _ in om]
            k = 0
            while k < min(of, len(omoves)) and omoves[k] == moves[k]:
                k += 1
            if k >= of:
                same = "same game up to that ply"
            else:
                who = "Regicide" if (k % 2 == 0) == (rc == "B") else "the opponent"
                same = f"deviated at ply {k} ({who}: {omoves[k] if k < len(omoves) else 'end'} vs {moves[k]})"
        res = {"B": "1-0", "W": "0-1", "D": "draw"}[r["res"]]
        md.append(f"| {r['run']} | {r['round']} | {r['ballot']} | {rc} | {of} | {pieces(fens[of])} | "
                  f"{sc_before if sc_before is not None else '-'} | "
                  f"{f'ply {first_neg[0]}, {first_neg[1]} pieces' if first_neg else 'never'} | {res} | {same} |")
        lst.append(f"{fens[of]} | {fens[max(0, of - 4)]} | {r['run']} {r['round']} {r['ballot']}, Regicide {rc}, "
                   f"{NAMES[opp]}, {res}")
    open(a.out, "w").write("\n".join(md) + "\n")
    open(a.list, "w").write("\n".join(lst) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
