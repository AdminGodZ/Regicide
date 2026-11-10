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
    ap.add_argument("--quiet", action="store_true",
                    help="count a database win only in a position where the side to move has no capture pending")
    ap.add_argument("--no-dedup", action="store_true",
                    help="keep identical games from different runs (comparisons between builds that can play the same game)")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    games, seen, dups, unfinished, conflicts = [], {}, 0, [], []
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
            # result first: an unfinished copy must not hide a finished copy of the same game as a "duplicate"
            res = {"1-0": "B", "0-1": "W", "1/2-1/2": "D"}.get(tags.get("Result", "").strip())
            if res is None:  # "*" or missing: interrupted or unfinished game, no final result
                unfinished.append((run, tags.get("Round", ""), tags.get("Result", "")))
                continue
            k = (bf, wf, tags.get("FEN", ""), mv) if not a.no_dedup else (run, tags.get("Round", ""))
            if k in seen:
                dups += 1
                first_rec = seen[k]
                if (first_rec[0], first_rec[1]) != (res, tags.get("Termination", "")):  # same moves, other outcome
                    conflicts.append((first_rec, (res, tags.get("Termination", ""), run, tags.get("Round", ""))))
                continue
            seen[k] = (res, tags.get("Termination", ""), run, tags.get("Round", ""))
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
        procs.append(subprocess.Popen([a.scan, a.egdb, "1024"] + (["quiet"] if a.quiet else []), stdin=open(inp),
                                      stdout=subprocess.PIPE, text=True))
    by_id = {id(g): g for g in games}
    rows, rejected = 0, []
    for p in procs:
        for line in p.stdout:
            f = line.rstrip("\n").split("\t")
            g = by_id[int(f[0])]
            rows += 1
            if f[1] == "ERR":  # a move that does not parse to exactly one legal move: reject the whole game
                g["rejected"] = f"ply {f[2]} token {f[3]}"
                rejected.append(g)
                continue
            g.update(first=int(f[1]), winner=f[2], pieces=int(f[3]), wfen=f[4], throw=int(f[5]), tmove=f[6],
                     tfen=f[7], throws=int(f[8]), final=f[9], fpieces=int(f[10]), plies=int(f[11]),
                     max_quiet=int(f[12]), first_b=int(f[13]), first_w=int(f[14]), traj=f[15] if len(f) > 15 else "")
        if p.wait() != 0:
            sys.exit(f"scanner exited with status {p.returncode}")
    if rows != len(games):
        sys.exit(f"scanner returned {rows} rows for {len(games)} games")
    for j in range(a.jobs):
        os.remove(os.path.join(a.out, f"scan_in_{j}.tsv"))
    games = [g for g in games if "first" in g]
    print(f"{len(rejected)} games rejected (unparsable or ambiguous move); {len(conflicts)} duplicate records with the "
          f"same moves but another result or termination", file=sys.stderr)

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
          f"result (\"*\" or none: interrupted or unfinished) skipped, {len(rejected)} games rejected (a move that does "
          f"not parse to exactly one legal move), {len(conflicts)} duplicate records with the same moves but another "
          f"result or termination (the first record is kept; listed at the end). Database: Regicide's 2–8-piece WLD.",
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
    if rejected or conflicts:
        md.append("## Rejected games and duplicate conflicts")
        md.append("")
        for g in rejected:
            md.append(f"- rejected: {g['run']} {g['round']} ({g['rejected']})")
        for kept, other in conflicts:
            md.append(f"- same moves, other outcome: kept {kept[2]} {kept[3]} ({kept[0]}, {kept[1]}); "
                      f"dropped {other[2]} {other[3]} ({other[0]}, {other[1]})")
        md.append("")
    open(os.path.join(a.out, "dbscan.md"), "w").write("\n".join(md) + "\n")
    with open(os.path.join(a.out, "missed_wins.txt"), "w") as fo:
        fo.write("# WP8 regression set: first database-won position (Regicide to move or just moved, <= 8 pieces) of every\n"
                 "# game in which Regicide had a database win and did not win. FEN | pieces | run round ballot | ending\n")
        for g in sorted(cats["regicide"], key=lambda g: (g["run"], g["round"])):
            fo.write(f"{g['wfen']} | {g['pieces']} | {g['run']} {g['round']} {g['ballot']} | {g['term']}; {how(g)}\n")
    with open(os.path.join(a.out, "dbscan_games.tsv"), "w") as fo:
        keys = ["run", "round", "ballot", "bf", "wf", "res", "term", "first", "winner", "pieces", "throw", "throws",
                "final", "fpieces", "plies", "max_quiet", "first_b", "first_w", "wfen", "traj"]
        fo.write("\t".join(keys) + "\n")
        for g in games:
            fo.write("\t".join(str(g[k]) for k in keys) + "\n")
    print("\n".join(md[:12]))


if __name__ == "__main__":
    main()
