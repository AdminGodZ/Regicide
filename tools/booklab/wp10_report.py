#!/usr/bin/env python3
"""WP10 holdout report.

  wp10_report.py --results ~/regicide-match/results --runs RUN... --changed changed_positions.tsv
                 --manifest experiments/holdout/manifest.json --replay booklab_replay --scan booklab_dbscan
                 --egdb DIR --out-md report.md --out-tsv games.tsv [--pair EB:O --pair EB:E ...]

Run names: bl-w10-b<B>-<CFG>-<opp>-<part> (part H1, H2, H3_0, H2x_1, ...), bl-f4-<opp>-<half> (EBN on acf156),
bl-f1-<opp>-<run>-<half> (final validation, EB with book r8 = r10 in play).
Per game: configuration, opponent, panel (H2x with its tier), opening, Regicide's colour, result, termination,
exposure (our side to move in one of the changed positions: --changed lines marked DIFFER) with the first such ply,
and the first in-play database win per side (booklab_dbscan, quiet positions, final position excluded).
"""
import argparse
import collections
import csv
import json
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

NAMES = {"cake": "Cake", "kingsrow": "KingsRow", "guinn": "GuiNN"}


def parse_run(run):
    p = run.split("-")
    if p[1] == "w10":   # bl-w10-b3-EB-cake-H3_0
        return p[3], p[4], p[5].split("_")[0], p[2]
    if p[1] == "f4":
        return "EBN", p[2], "acf156", "b2"
    if p[1] == "f1":
        return "EB(final,r8)", p[2], "acf156", "final"
    return "?", "?", "?", "?"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--changed", required=True)
    ap.add_argument("--manifest", required=True)
    ap.add_argument("--replay", required=True)
    ap.add_argument("--scan", required=True)
    ap.add_argument("--egdb", required=True)
    ap.add_argument("--out-md", required=True)
    ap.add_argument("--out-tsv", required=True)
    ap.add_argument("--pair", action="append", default=[])
    a = ap.parse_args()
    changed = {l.split("\t")[0] for l in open(a.changed) if l.rstrip().endswith("DIFFER")}
    man = json.load(open(a.manifest))
    tier = {o["moves"]: o["tier"] for o in man["panels"]["h2x_touched156.txt"]["openings"]}
    games = []
    for run in a.runs:
        f = os.path.join(a.results, run, "games.pdn")
        if not os.path.exists(f):
            continue
        cfg, opp, panel, block = parse_run(run)
        for tags, moves in parse_pdn(open(f, errors="replace").read()):
            bf, wf = family(tags.get("Black", "")), family(tags.get("White", ""))
            col = "B" if bf == "regicide" else "W"
            res = tags.get("Result", "")
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
            _, om, nopen = ballot_of(tags.get("Opening", ""))
            rnd = tags.get("Round", "0.0").split(".")
            gfile = f"{run}/games/{(int(rnd[0]) - 1) * 2 + int(rnd[1]):05d}.pdn"
            pn = panel
            if panel == "H2x":
                pn = f"H2x-t{tier.get(om, '?')}"
            games.append(dict(run=run, block=block, cfg=cfg, opp=opp, panel=pn, opening=om, col=col, res=r,
                              term=tags.get("Termination", ""), file=gfile, fen=tags.get("FEN", ""),
                              moves=[m for m, _ in moves]))
    # exposure
    inp = "".join(f"{i}\t{g['fen']}\t{' '.join(g['moves'])}\n" for i, g in enumerate(games))
    out = subprocess.run([a.replay], input=inp, capture_output=True, text=True, check=True).stdout
    fen_of = {}
    for l in out.splitlines():
        p = l.split("\t")
        if p[0] == "F":
            fen_of[p[1]] = p[2]
    for g in games:
        g["exposed"] = -1
    for l in out.splitlines():
        p = l.split("\t")
        if p[0] != "P":
            continue
        g = games[int(p[1])]
        fen = fen_of[p[3]]
        if g["exposed"] < 0 and fen[0].upper() == g["col"] and fen in changed:
            g["exposed"] = int(p[2])
    # database wins in play
    out = subprocess.run([a.scan, a.egdb, "1024", "quiet"], input=inp, capture_output=True, text=True,
                         check=True).stdout
    for l in out.splitlines():
        f = l.split("\t")
        g = games[int(f[0])]
        if f[1] == "ERR":
            g["db_reg"] = g["db_opp"] = "ERR"
            continue
        fb, fw = int(f[13]), int(f[14])
        g["db_reg"] = (fb if g["col"] == "B" else fw)
        g["db_opp"] = (fw if g["col"] == "B" else fb)
    keys = ["block", "cfg", "opp", "panel", "opening", "col", "res", "term", "exposed", "db_reg", "db_opp", "file"]
    with open(a.out_tsv, "w") as fo:
        fo.write("\t".join(keys) + "\n")
        for g in games:
            fo.write("\t".join(str(g[k]) for k in keys) + "\n")
    # tables
    md = [f"# WP10 report: {len(games)} games", ""]
    md.append("| Block | Config | Opponent | Panel | Games | W / D / L | exposed games: W/D/L | unexposed: W/D/L | "
              "Regicide db wins in play: converted | opponent db wins: converted | forfeits |")
    md.append("|---|---|---|---|---|---|---|---|---|---|---|")
    grp = collections.defaultdict(list)
    for g in games:
        grp[(g["block"], g["cfg"], g["opp"], g["panel"])].append(g)

    def wdl(gs):
        c = collections.Counter(x["res"] for x in gs)
        return f"{c['W']} / {c['D']} / {c['L']}"
    for k in sorted(grp):
        gs = grp[k]
        ex = [x for x in gs if x["exposed"] >= 0]
        un = [x for x in gs if x["exposed"] < 0]
        rr = [x for x in gs if isinstance(x["db_reg"], int) and x["db_reg"] >= 0]
        orr = [x for x in gs if isinstance(x["db_opp"], int) and x["db_opp"] >= 0]
        ff = sum(1 for x in gs if any(w in x["term"].lower() for w in ("illegal", "time", "crash", "forfeit")))
        md.append(f"| {k[0]} | {k[1]} | {NAMES.get(k[2], k[2])} | {k[3]} | {len(gs)} | {wdl(gs)} | "
                  f"{len(ex)}: {wdl(ex)} | {len(un)}: {wdl(un)} | {sum(x['res'] == 'W' for x in rr)} / {len(rr)} | "
                  f"{sum(x['res'] == 'L' for x in orr)} / {len(orr)} | {ff} |")
    md += ["", "## Losses", "", "| Block | Config | Opponent | Panel | Opening | Regicide as | Termination | Exposed at ply | Game |",
           "|---|---|---|---|---|---|---|---|---|"]
    for g in games:
        if g["res"] == "L":
            md.append(f"| {g['block']} | {g['cfg']} | {NAMES.get(g['opp'], g['opp'])} | {g['panel']} | {g['opening']} | "
                      f"{g['col']} | {g['term']} | {g['exposed'] if g['exposed'] >= 0 else '-'} | `{g['file']}` |")
    # H1 by colour pair
    h1 = [g for g in games if g["panel"] == "H1"]
    if h1:
        md += ["", "## H1 (barred ballots) by colour pair: Regicide's result as Black / as White", "",
               "| Config | Opponent | Opening | as Black | as White |", "|---|---|---|---|---|"]
        cp = collections.defaultdict(dict)
        for g in h1:
            cp[(g["cfg"], g["opp"], g["opening"])][g["col"]] = g["res"]
        for k in sorted(cp):
            md.append(f"| {k[0]} | {NAMES.get(k[1], k[1])} | {k[2]} | {cp[k].get('B', '-')} | {cp[k].get('W', '-')} |")
    # pairings
    pts = {"W": 1, "D": 0.5, "L": 0}
    for pr in a.pair:
        x, y = pr.split(":")
        idx = collections.defaultdict(dict)
        for g in games:
            idx[(g["opp"], g["panel"], g["opening"], g["col"])][g["cfg"]] = g
        rows = collections.defaultdict(lambda: collections.Counter())
        for k, d in idx.items():
            if x in d and y in d:
                diff = pts[d[x]["res"]] - pts[d[y]["res"]] if d[x]["res"] in pts and d[y]["res"] in pts else 0
                key = (k[0], k[1], "exposed" if d[x]["exposed"] >= 0 else "unexposed")
                rows[key]["n"] += 1
                rows[key]["better" if diff > 0 else "worse" if diff < 0 else "same"] += 1
                rows[key]["diff"] += diff
        if rows:
            md += ["", f"## Paired by opening and colour: {x} minus {y}", "",
                   f"| Opponent | Panel | {x} game exposed? | Pairs | {x} better | same | {x} worse | score difference |",
                   "|---|---|---|---|---|---|---|---|"]
            for k in sorted(rows):
                c = rows[k]
                md.append(f"| {NAMES.get(k[0], k[0])} | {k[1]} | {k[2]} | {c['n']} | {c['better']} | {c['same']} | "
                          f"{c['worse']} | {c['diff']:+.1f} |")
    open(a.out_md, "w").write("\n".join(md) + "\n")
    print("\n".join(md[:40]))


if __name__ == "__main__":
    main()
