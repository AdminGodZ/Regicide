#!/usr/bin/env python3
"""WP8: missed database wins.

  dbscan.py --results ~/regicide-match/results --scan booklab_dbscan --egdb DIR --out OUTDIR [--jobs 4]

Takes every games.pdn under --results (one level of run directories), keeps games Regicide played against Cake,
KingsRow or GuiNN and games among those three (mirrors included), removes identical games (same engine families and
the same moves), replays each with booklab_dbscan (database probes at <= 8 pieces) and writes:
  OUTDIR/dbscan.md            totals and the three tables
  OUTDIR/missed_wins.txt      FEN of the first won position of every game Regicide had won and did not win
  OUTDIR/dbscan_games.tsv     one row per scanned game
"""
import argparse
import collections
import glob
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

TOP = ("cake", "kingsrow", "guinn")
NAMES = {"cake": "Cake", "kingsrow": "KingsRow", "guinn": "GuiNN", "regicide": "Regicide"}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--scan", required=True)
    ap.add_argument("--egdb", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--jobs", type=int, default=4)
    ap.add_argument("--exclude", default="bl-wp9",
                    help="comma-separated run-name prefixes to leave out (default: the WP9 conversion tests, which "
                         "start from database-won positions)")
    ap.add_argument("--until", help="only runs whose games.pdn was last modified before this time (YYYY-MM-DDTHH:MM)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    games, seen, dups, unfinished = [], set(), 0, []
    excl = tuple(x for x in a.exclude.split(",") if x)
    until = None
    if a.until:
        import datetime
        until = datetime.datetime.fromisoformat(a.until).timestamp()
    for f in sorted(glob.glob(os.path.join(a.results, "*", "games.pdn"))):
        run = os.path.basename(os.path.dirname(f))
        if (excl and run.startswith(excl)) or (until and os.path.getmtime(f) >= until):
            continue
        for tags, moves in parse_pdn(open(f, errors="replace").read()):
            bf, wf = family(tags.get("Black", "")), family(tags.get("White", ""))
            if not ((bf in TOP and wf in TOP) or (bf == "regicide" and wf in TOP) or (wf == "regicide" and bf in TOP)):
                continue
            mv = " ".join(m for m, _ in moves)
            k = (bf, wf, tags.get("FEN", ""), mv)
            if k in seen:
                dups += 1
                continue
            seen.add(k)
            res = {"1-0": "B", "0-1": "W", "1/2-1/2": "D"}.get(tags.get("Result", "").strip())
            if res is None:  # "*" or missing: interrupted or unfinished game, no final result
                unfinished.append((run, tags.get("Round", ""), tags.get("Result", "")))
                continue
            games.append(dict(run=run, round=tags.get("Round", ""), ballot=ballot_of(tags.get("Opening", ""))[0],
                              bf=bf, wf=wf, res=res, term=tags.get("Termination", ""), fen=tags.get("FEN", ""), mv=mv))
    print(f"{len(games)} games ({dups} duplicates removed, {len(unfinished)} games without a final result skipped)",
          file=sys.stderr)
    # scan in parallel chunks
    chunks = [games[i::a.jobs] for i in range(a.jobs)]
    procs = []
    for j, ch in enumerate(chunks):
        inp = os.path.join(a.out, f"scan_in_{j}.tsv")
        with open(inp, "w") as fo:
            for g in ch:
                fo.write(f"{games.index(g) if False else id(g)}\t{g['fen']}\t{g['mv']}\n")
        procs.append(subprocess.Popen([a.scan, a.egdb, "1024"], stdin=open(inp), stdout=subprocess.PIPE, text=True))
    by_id = {id(g): g for g in games}
    for p in procs:
        for line in p.stdout:
            f = line.rstrip("\n").split("\t")
            g = by_id[int(f[0])]
            g.update(first=int(f[1]), winner=f[2], pieces=int(f[3]), wfen=f[4], throw=int(f[5]), tmove=f[6],
                     tfen=f[7], throws=int(f[8]), final=f[9], fpieces=int(f[10]), plies=int(f[11]),
                     max_quiet=int(f[12]))
        p.wait()
    for j in range(a.jobs):
        os.remove(os.path.join(a.out, f"scan_in_{j}.tsv"))
    games = [g for g in games if "first" in g]

    def side_fam(g, col):
        return g["bf"] if col == "B" else g["wf"]

    def how(g):
        if g["throw"] >= 0:
            return (f"winner moved into a non-won position at ply {g['throw']} ({g['tmove']}, "
                    f"{g['throws']} such move{'s' if g['throws'] != 1 else ''}); final db value {g['final']}")
        return f"still won at the end (final db value {g['final']}, {g['fpieces']} pieces): ran out the rules"

    cats = {"regicide": [], "opponent": [], "census": []}
    tot = collections.Counter()
    for g in games:
        kind = "census" if (g["bf"] in TOP and g["wf"] in TOP) else "regicide_game"
        tot[(kind, "scanned")] += 1
        if g["first"] < 0:
            continue
        tot[(kind, "reached")] += 1
        won = g["res"] == g["winner"]
        tot[(kind, "converted")] += won
        if won:
            continue
        wfam = side_fam(g, g["winner"])
        if kind == "census":
            cats["census"].append(g)
        elif wfam == "regicide":
            cats["regicide"].append(g)
        else:
            cats["opponent"].append(g)
        tot[(kind, "missed_" + ("regicide" if wfam == "regicide" else "opp"))] += 1
    md = ["# WP8: missed database wins", "",
          f"Games: every games.pdn under `{a.results}` (Regicide vs Cake/KingsRow/GuiNN, all engine versions, plus the "
          f"games among the three opponents), {dups} duplicate games removed, {len(unfinished)} games without a final "
          f"result (\"*\" or none: interrupted or unfinished) skipped. Database: Regicide's 2–8-piece WLD.",
          "A game *reached a database win* when some position with <= 8 pieces is a database win for one side.", ""]
    md.append("| Games | Scanned | Reached a database win | Winning side won | Not won |")
    md.append("|---|---|---|---|---|")
    for kind, label in (("regicide_game", "Regicide vs Cake/KingsRow/GuiNN"), ("census", "Opponent vs opponent")):
        r, c = tot[(kind, "reached")], tot[(kind, "converted")]
        md.append(f"| {label} | {tot[(kind, 'scanned')]} | {r} | {c} | {r - c} |")
    per = collections.Counter()
    for g in games:
        if g["first"] >= 0:
            fam = side_fam(g, g["winner"])
            per[(fam, "reached")] += 1
            per[(fam, "won")] += g["res"] == g["winner"]
    md.append("")
    md.append("Conversion by the side that had the database win: " + "; ".join(
        f"{NAMES[f]} {per[(f, 'won')]}/{per[(f, 'reached')]}" for f in ("regicide", "cake", "kingsrow", "guinn")
        if per[(f, "reached")]))
    md.append("")

    def table(title, rows):
        md.append(f"## {title} ({len(rows)})")
        md.append("")
        md.append("| Run | Round | Ballot | Winner side | Against | First won ply | Pieces | Result | Termination | "
                  "How it was lost | FEN of the first won position |")
        md.append("|---|---|---|---|---|---|---|---|---|---|---|")
        for g in sorted(rows, key=lambda g: (g["run"], g["round"])):
            wcol = g["winner"]
            md.append(f"| {g['run']} | {g['round']} | {g['ballot']} | {NAMES[side_fam(g, wcol)]} as "
                      f"{'Black' if wcol == 'B' else 'White'} | {NAMES[side_fam(g, 'W' if wcol == 'B' else 'B')]} | "
                      f"{g['first']} | {g['pieces']} | {g['res']} | {g['term']} | {how(g)} | `{g['wfen']}` |")
        md.append("")

    table("1. Regicide had a database win and did not win", cats["regicide"])
    table("2. An opponent had a database win against Regicide and did not win", cats["opponent"])
    table("3. Opponent vs opponent: database win not converted", cats["census"])
    open(os.path.join(a.out, "dbscan.md"), "w").write("\n".join(md) + "\n")
    with open(os.path.join(a.out, "missed_wins.txt"), "w") as fo:
        fo.write("# WP8 regression set: first database-won position (Regicide to move or just moved, <= 8 pieces) of every\n"
                 "# game in which Regicide had a database win and did not win. FEN | pieces | run round ballot | ending\n")
        for g in sorted(cats["regicide"], key=lambda g: (g["run"], g["round"])):
            fo.write(f"{g['wfen']} | {g['pieces']} | {g['run']} {g['round']} {g['ballot']} | {g['term']}; {how(g)}\n")
    with open(os.path.join(a.out, "dbscan_games.tsv"), "w") as fo:
        keys = ["run", "round", "ballot", "bf", "wf", "res", "term", "first", "winner", "pieces", "throw", "throws",
                "final", "fpieces", "plies", "max_quiet", "wfen"]
        fo.write("\t".join(keys) + "\n")
        for g in games:
            fo.write("\t".join(str(g[k]) for k in keys) + "\n")
    print("\n".join(md[:12]))


if __name__ == "__main__":
    main()
