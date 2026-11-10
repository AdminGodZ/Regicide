#!/usr/bin/env python3
"""WP12: candidate points on the lines the pinned book really plays against Cake (PROTOCOL.md on master e670910).

  wp12_points.py --runs DIR... --context-runs DIR... --book book_r11.bin --pinwalk booklab_pinwalk --egdb DIR
                 --out points.tsv

--runs: the games of the r11 validation against Cake with n6c (v11-n6c-cake-h1, -h2). A point is our move position
on such a game that
  - comes before the first position with 8 or fewer pieces,
  - where the engine consults the book (no run of king moves, root not database-decided: pinwalk's eligibility), and
    the book (r11) plays the move that was played (the point is on the r11 line),
  - (at least two legal non-avoided moves: checked by the scoring step, which sees all moves).
Each point gets its distance in plies from the end of Cake's book in that game (the last Cake move its own comment
marks as book) and a category: 2 = 10..25 plies after Cake's book ends, 3 = earlier, 4 = later (not used).
Points are deduplicated by position (the first game met keeps it, sorted runs, games in file order).
--context-runs: every validation game (any opponent, any run); for every point, every (ballot, colour, opponent,
run:round) whose game reaches the point's position with Regicide to move is listed (protocol: every ballot and
colour from which a candidate's position is reached is recorded and screened in stage 3).
Output TSV: id, category, distance, ballot, colour, run, round, ply, fen, pinned move, contexts
(contexts: "ballot|colour|opponent|run:round|ply" separated by ";", the point's own game included; ply = the
position's ply in that game, from the start position).
"""
import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of, is_book  # noqa: E402


def load_games(dirs):
    games = []
    for d in sorted(dirs):
        run = os.path.basename(d.rstrip("/"))
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            col = "B" if family(tags["Black"]) == "regicide" else "W"
            opp = family(tags["White"] if col == "B" else tags["Black"])
            bname, bm, nopen = ballot_of(tags["Opening"])
            games.append(dict(run=run, rnd=tags["Round"], col=col, opp=opp, ballot=bname, nopen=nopen,
                              fen0=tags.get("FEN", ""), moves=[m for m, _ in moves], comments=[c for _, c in moves]))
    return games


def pinwalk(exe, book, egdb, games):
    lines = [f"{i}\t{g['fen0']}\t{g['col']}\t{g['nopen']}\t{' '.join(g['moves'])}" for i, g in enumerate(games)]
    out = subprocess.run([exe, book, egdb], input="\n".join(lines) + "\n", capture_output=True, text=True,
                         check=True).stdout
    rows = []
    for l in out.splitlines():
        f = l.split("\t")
        if f[1] == "ERR":
            sys.exit(f"pinwalk: {l}")
        rows.append((int(f[0]), int(f[1]), f[2], f[3], f[4] == "1", f[5]))
    return rows


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--context-runs", nargs="+", required=True)
    ap.add_argument("--book", required=True)
    ap.add_argument("--pinwalk", required=True)
    ap.add_argument("--egdb", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    games = load_games(a.runs)
    for g in games:
        if g["opp"] != "cake":
            sys.exit(f"--runs must be games against Cake: {g['run']} round {g['rnd']} is against {g['opp']}")
        g["cbe"] = g["nopen"] - 1
        for i in range(g["nopen"], len(g["moves"])):
            if (i % 2 == 0) != (g["col"] == "B") and g["comments"][i] and is_book("cake", g["comments"][i]):
                g["cbe"] = i
    pts = {}
    for gi, ply, fen, mv, el, bmv in pinwalk(a.pinwalk, a.book, a.egdb, games):
        g = games[gi]
        if not el or bmv != mv or fen in pts:
            continue
        dist = ply - g["cbe"]
        cat = 2 if 10 <= dist <= 25 else (3 if dist < 10 else 4)
        pts[fen] = (cat, dist, g["ballot"], g["col"], g["run"], g["rnd"], ply, fen, mv)
    # contexts: every validation game reaching a point's position with Regicide to move
    cgames = load_games(a.context_runs)
    ctx = collections.defaultdict(set)
    for gi, ply, fen, mv, el, bmv in pinwalk(a.pinwalk, a.book, a.egdb, cgames):
        if fen in pts:
            g = cgames[gi]
            ctx[fen].add(f"{g['ballot']}|{g['col']}|{g['opp']}|{g['run']}:{g['rnd']}|{ply}")
    for fen, p in pts.items():  # the point's own game is always a context
        ctx[fen].add(f"{p[2]}|{p[3]}|cake|{p[4]}:{p[5]}|{p[6]}")
    with open(a.out, "w") as fo:
        fo.write("id\tcategory\tdistance\tballot\tcolour\trun\tround\tply\tfen\tpinned\tcontexts\n")
        for i, (fen, p) in enumerate(sorted(pts.items(), key=lambda kv: (kv[1][0], kv[1][2], kv[1][3], kv[1][6]))):
            fo.write(f"P{i + 1}\t" + "\t".join(map(str, p)) + "\t" + ";".join(sorted(ctx[fen])) + "\n")
    c = collections.Counter(p[0] for p in pts.values())
    multi = sum(1 for f in pts if len({x.split("|")[0] + x.split("|")[1] for x in ctx[f]}) > 1)
    print(f"{len(games)} Cake games, {len(pts)} distinct points: category 2: {c[2]}, 3: {c[3]}, 4: {c[4]}; "
          f"{multi} points reached from more than one ballot/colour in {len(cgames)} validation games")


if __name__ == "__main__":
    main()
