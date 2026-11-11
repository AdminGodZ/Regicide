#!/usr/bin/env python3
"""WP14 export for the search lab (lead 2026-11-11 14:32): positions where our 0.5 s game search and our 100M root
differ by 50 or more.

  wp14_dips.py --scores step1.out c6low.out --tasks tasks.tsv c6low.tsv --contexts contexts.tsv --results DIR
               --out dips.tsv [--min 50]
For every scored position (rootscore output: root lines) whose game move was a search (Regicide's PDN comment
"score N" with nodes > 0): the game's score and depth at that move, the 100M root score, best move and depth; rows
with |game - 100M| >= MIN. Columns: fen, side to move, moves_from_start (the full history: replaying it from the
initial position gives the position and every earlier one, so repetition counts are exact), game_score, game_depth,
score_100m, best_100m, depth_100m, move_played, opponent, opening, run, round, ply. FENs from booklab_replay.
"""
import argparse
import csv
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of, family  # noqa: E402

REPLAY = os.path.expanduser("~/regicide-booklab/build-linux/booklab_replay")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--scores", nargs="+", required=True)
    ap.add_argument("--tasks", nargs="+", required=True)
    ap.add_argument("--contexts", required=True)
    ap.add_argument("--results", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--min", type=int, default=50)
    a = ap.parse_args()
    root = {}
    for f in a.scores:
        for l in open(f):
            x = l.rstrip("\n").split("\t")
            if len(x) > 4 and x[1] == "root" and x[3] != "none":
                root[x[0]] = (x[2], int(x[3]), int(x[4]))
    moves = {}
    for f in a.tasks:
        for l in open(f):
            x = l.rstrip("\n").split("\t")
            moves[x[0]] = x[2]
    # which game each task comes from: the near-miss contexts, and the c6 low points (id low<k>:<arm>:<ply>:<score>)
    src = {}
    for c in csv.DictReader(open(a.contexts), delimiter="\t"):
        for t in c["tasks"].split():
            tid, ply, mv, kind = t.split(":")
            src.setdefault(tid, (c["run"], c["round"], int(ply)))
    games = {}

    def game(run, rnd):
        if (run, rnd) not in games:
            for t, m in parse_pdn(open(os.path.join(a.results, "bl-" + run, "games.pdn"), errors="replace").read()):
                games[(run, t["Round"])] = (t, m)
        return games[(run, rnd)]

    rows = []
    for tid, (best, s100, d100) in root.items():
        mv_list = moves[tid].split()
        if tid in src:
            run, rnd, ply = src[tid]
            t, m = game(run, rnd)
        else:  # c6 low points: find a w14c6 game with this move list
            ply = len(mv_list) + 1
            t = m = None
            for run in ("w14c6-kingsrow-fresh", "w14c6-kingsrow-kept"):
                for tt, mm in parse_pdn(open(os.path.join(a.results, "bl-" + run, "games.pdn"), errors="replace").read()):
                    if [x for x, _ in mm[:ply - 1]] == mv_list:
                        t, m, rnd = tt, mm, tt["Round"]
                        break
                if t:
                    break
        c = m[ply - 1][1] or ""
        sc, dp, nd = re.search(r"\bscore (-?\d+)", c), re.search(r"depth (\d+)", c), re.search(r"nodes (\d+)", c)
        if not sc or not nd or nd.group(1) == "0":
            continue  # a book move: no game search to compare
        g = int(sc.group(1))
        if abs(g - s100) < a.min:
            continue
        rb = t["Black"].lower().startswith("regicide")
        rows.append(dict(moves=" ".join(mv_list), game_score=g, game_depth=int(dp.group(1)) if dp else "", score_100m=s100,
                         best_100m=best, depth_100m=d100, move_played=m[ply - 1][0],
                         opponent=family(t["White"] if rb else t["Black"]), opening=ballot_of(t["Opening"])[0],
                         run=run, round=rnd, ply=ply))
    seen, uniq = set(), []
    for r in rows:  # the same position from several games: one row, the games listed
        if r["moves"] in seen:
            next(u for u in uniq if u["moves"] == r["moves"])["run"] += f",{r['run']}:{r['round']}"
            continue
        seen.add(r["moves"])
        r["run"] = f"{r['run']}:{r['round']}"
        uniq.append(r)
    rows = uniq
    inp = "".join(f"{i}\t\t{r['moves']}\n" for i, r in enumerate(rows))
    out = subprocess.run([REPLAY], input=inp, capture_output=True, text=True, check=True).stdout
    fen = {int(l.split("\t")[1]): l.split("\t")[3] for l in out.splitlines() if l.startswith("Z\t")}
    # legal moves at each position (multiscore NODES 0 lists them): a forced move is played at once with a depth-1
    # score, which is what the PDN comment then holds
    ms = os.path.expanduser("~/regicide-jobs/booklab/bin/blk-multiscore-1c6e")
    lm = subprocess.run([ms, "0", "1"], input="".join(f"{i}\t{fen.get(i, '')}\n" for i in range(len(rows))),
                        capture_output=True, text=True, check=True).stdout
    nlegal = {}
    for l in lm.splitlines():
        nlegal[int(l.split("\t")[0])] = nlegal.get(int(l.split("\t")[0]), 0) + 1
    cols = ["fen", "side_to_move", "legal_moves", "moves_from_start", "game_score", "game_depth", "score_100m", "best_100m",
            "depth_100m", "move_played", "opponent", "opening", "games (run:round)", "ply"]
    with open(a.out, "w") as fo:
        fo.write("\t".join(cols) + "\n")
        for i, r in sorted(enumerate(rows), key=lambda ir: ir[1]["game_score"] - ir[1]["score_100m"]):
            f = fen.get(i, "?")
            fo.write("\t".join(str(v) for v in [f, f[:1], nlegal.get(i, "?"), r["moves"], r["game_score"], r["game_depth"],
                                                r["score_100m"], r["best_100m"], r["depth_100m"], r["move_played"],
                                                r["opponent"], r["opening"], r["run"], r["ply"]]) + "\n")
    print(f"{len(rows)} positions with |game - 100M| >= {a.min}")


if __name__ == "__main__":
    main()
