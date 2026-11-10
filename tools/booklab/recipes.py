#!/usr/bin/env python3
"""WP7b: win recipes -> force decisions.

  recipes.py --wins wins_census.tsv --decisions decisions_r5.txt --db theory.sqlite --replay booklab_replay
             [--extra 4] [--extend FILE] --out recipes.txt --report recipes_report.tsv

For each decisive game (wins.py output; win ply known): the winner's moves from the end of the ballot up to its win ply
plus --extra more winner moves (capped at the game's end) become force decisions for the winner's colour.
--extend FILE: lines "wN PLIES" add PLIES more plies (the 6-ply extension of WP7c) to recipe wN.
Conflicts, reported and not overwritten:
  - repair: the position already has a different decision in --decisions (the repair wins);
  - recipe: two recipes force different moves in the same position (the earlier recipe in file order wins; the later
    recipe is marked broken from that ply on).
Notes (not blocking): positions where Regicide (any version, theory table) played another move there and never lost
with it in at least one game, i.e. the recipe replaces a move that drew; listed per recipe in the report.
"""
import argparse
import os
import sqlite3
import subprocess
import sys


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--wins", required=True)
    ap.add_argument("--decisions", required=True)
    ap.add_argument("--db", required=True)
    ap.add_argument("--replay", required=True)
    ap.add_argument("--extra", type=int, default=4)
    ap.add_argument("--extend")
    ap.add_argument("--only", help="comma-separated recipe ids to emit (default all)")
    ap.add_argument("--out", required=True)
    ap.add_argument("--report", required=True)
    a = ap.parse_args()
    ext = {}
    if a.extend and os.path.exists(a.extend):
        for line in open(a.extend):
            if line.strip() and not line.startswith("#"):
                k, v = line.split()[:2]
                ext[k] = int(v)
    only = set(a.only.split(",")) if a.only else None
    existing = {}
    for line in open(a.decisions):
        if line.startswith("#") or line.count(" | ") < 3:
            continue
        f = line.split(" | ")
        existing[f[0].strip()] = f[1].strip()
    recs = []
    for line in open(a.wins):
        f = line.rstrip("\n").split("\t")
        rid, run, rnd, ballot, bmoves, wf, wname, lf, col, nopen, win_ply, nplies, moves = f
        if win_ply == "None" or (only and rid not in only):
            continue
        recs.append(dict(id=rid, run=run, round=rnd, ballot=ballot, wf=wf, lf=lf, col=col, nopen=int(nopen),
                         win=int(win_ply), n=int(nplies), moves=moves.split()))
    # FENs and keys of every position on every recipe line.
    inp = "".join(f"{i}\t\t{' '.join(r['moves'])}\n" for i, r in enumerate(recs))
    out = subprocess.run([a.replay], input=inp, capture_output=True, text=True, check=True).stdout
    fen_of, key_at = {}, {}
    for line in out.splitlines():
        p = line.split("\t")
        if p[0] == "F":
            fen_of[p[1]] = p[2]
        elif p[0] == "P":
            key_at[(int(p[1]), int(p[2]))] = p[3]
    db = sqlite3.connect(a.db)
    forced = {}  # fen -> (move, recipe id)
    lines, rep = [], []
    for i, r in enumerate(recs):
        last = min(r["win"] + 2 * a.extra + ext.get(r["id"], 0), r["n"] - 1)
        par = 0 if r["col"] == "B" else 1
        status, broken_at, n_dec, notes = "ok", None, 0, []
        dec = []
        for ply in range(r["nopen"], last + 1):
            if ply % 2 != par:
                continue
            key = key_at.get((i, ply))
            if key is None:
                status = "replay error"
                break
            fen, mv = fen_of[key], r["moves"][ply]
            # canonical move text as the replay prints it is not needed: decisions accept PDN notation
            if fen in existing and existing[fen] != mv:
                status, broken_at = f"conflict with repair at ply {ply} ({existing[fen]} vs {mv})", ply
                break
            if fen in forced and forced[fen][0] != mv:
                status, broken_at = (f"conflict with recipe {forced[fen][1]} at ply {ply} "
                                     f"({forced[fen][0]} vs {mv})"), ply
                break
            reg = db.execute("SELECT move, n, w, d, l FROM theory WHERE key=? AND fam='regicide'", (key,)).fetchall()
            drew = [(m, n, l) for m, n, w, d, l in reg if l == 0 and m != mv.replace("x", "x")]
            # theory moves are canonical (from-to); compare on from and to squares
            def ft(s):
                q = s.replace("x", "-").split("-")
                return q[0], q[-1]
            drew = [(m, n, l) for m, n, l in drew if ft(m) != ft(mv)]
            if drew:
                notes.append(f"ply {ply}: replaces regicide " + ",".join(f"{m} {n}x" for m, n, l in drew))
            dec.append((fen, mv, ply))
        for fen, mv, ply in dec:
            if fen not in forced and fen not in existing:
                forced[fen] = (mv, r["id"])
                lines.append(f"{fen} | {mv} | force | recipe {r['id']} {r['ballot']} {r['col']}: {r['wf']} beat {r['lf']} "
                             f"as {r['col']} (census {r['run']} round {r['round']}); ply {ply}, its win ply {r['win']} "
                             f"(own score >= +150), line forced to ply {last}")
                n_dec += 1
        rep.append([r["id"], r["ballot"], r["col"], r["wf"], r["lf"], r["win"], last, n_dec, status,
                    "; ".join(notes[:6]) + (f" (+{len(notes) - 6} more)" if len(notes) > 6 else "")])
    with open(a.out, "w") as f:
        f.write("# WP7 win recipes (recipes.py): the winner's moves of decisive census games, ballot to win ply + "
                f"{a.extra} winner moves\n")
        f.write("\n".join(lines) + "\n")
    with open(a.report, "w") as f:
        f.write("id\tballot\tcolour\twinner\tloser\twin_ply\tlast_ply\tdecisions\tstatus\treplaces_regicide_drawing_moves\n")
        for row in rep:
            f.write("\t".join(map(str, row)) + "\n")
    print(f"{len(recs)} recipes, {len(lines)} force decisions; "
          f"{sum(1 for r in rep if r[8] != 'ok')} with conflicts", file=sys.stderr)


if __name__ == "__main__":
    main()
