#!/usr/bin/env python3
"""WP7a: decisive census games as win recipes.

  wins.py [--show] [--thresh 150] RESULTDIR...
For every decisive game (winner X, loser Y; mirror games included): ballot, X's colour, the moves, and the "win
ply": the first ply of X's after the ballot from which X's own reported score stays >= --thresh for the rest of the
game (scores from the PDN comments: Cake/KingsRow 'value=', GuiNN 'Eval:', Regicide 'score'; book moves count as no
score). If the winner never reports such a score, win ply = None (then a Regicide search decides; see winply.sh).
Writes TSV to stdout: id, run, round, ballot, ballot_moves, winner_family, winner_name, loser_family, colour,
nopen, win_ply, plies, moves (space separated, from the start position).
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of, score_of, is_book  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--thresh", type=int, default=150)
    ap.add_argument("--show", action="store_true", help="print the winner's score series")
    ap.add_argument("dirs", nargs="+")
    a = ap.parse_args()
    n = 0
    for d in a.dirs:
        run = os.path.basename(d.rstrip("/"))
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            res = tags.get("Result", "")
            if res not in ("1-0", "0-1"):
                continue
            col = "B" if res == "1-0" else "W"
            wname = tags["Black"] if col == "B" else tags["White"]
            lname = tags["White"] if col == "B" else tags["Black"]
            wf, lf = family(wname), family(lname)
            bname, bmoves, nopen = ballot_of(tags.get("Opening", ""))
            scores = []  # (ply, score or None) for the winner's moves after the ballot
            for ply in range(nopen, len(moves)):
                if ("B" if ply % 2 == 0 else "W") != col:
                    continue
                c = moves[ply][1]
                s = None if (c is None or is_book(wf, c)) else score_of(c)
                if s is not None and wf in ("kingsrow", "guinn") and col == "W":
                    s = -s  # KingsRow and GuiNN report scores from Black's point of view
                scores.append((ply, s))
            win_ply = None
            for i, (ply, s) in enumerate(scores):
                later = [x for _, x in scores[i:] if x is not None]
                if s is not None and s >= a.thresh and sum(x >= a.thresh for x in later) >= 0.8 * len(later):
                    win_ply = ply
                    break
            n += 1
            if a.show:
                print(f"# {run} {tags.get('Round')} {bname} {wf} beats {lf} as {col}: win ply {win_ply}; "
                      + " ".join(f"{p}:{s}" for p, s in scores), file=sys.stderr)
            print("\t".join(map(str, [f"w{n}", run, tags.get("Round", ""), bname, bmoves, wf, wname, lf, col, nopen,
                                      win_ply, len(moves), " ".join(m for m, _ in moves)])))


if __name__ == "__main__":
    main()
