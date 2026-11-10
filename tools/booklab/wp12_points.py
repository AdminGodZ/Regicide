#!/usr/bin/env python3
"""WP12: candidate points on the validated lines against Cake (protocol: experiments/booklab/wp12/PROTOCOL.md).

  wp12_points.py --runs DIR... --book book_r11.bin --pinwalk booklab_pinwalk --egdb DIR --out points.tsv

Lines: the final-validation games against Cake (runs a and b). A point is our move position on the line that
  - comes before the first position with 8 or fewer pieces,
  - where the engine consults the book (no run of king moves, not database-decided: pinwalk's eligibility), and the
    book (r11) plays the move that was played (the point is on the r11 line),
  - with at least two legal moves (the avoid-list condition is checked by the analysis step, which sees all moves).
Each point gets its distance in plies from the end of Cake's book in that game (the last Cake move its own comment
marks as book) and a category: 2 = 10..25 plies after Cake's book ends, 3 = earlier, 4 = later (not used by the
protocol). Points are deduplicated by position (first line met keeps it; the other lines are listed).
Output TSV: id, category, distance, ballot, colour, run, round, ply, fen, pinned move, other lines.
"""
import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of, is_book  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--book", required=True)
    ap.add_argument("--pinwalk", required=True)
    ap.add_argument("--egdb", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    games, lines = [], []
    for d in sorted(a.runs):
        run = os.path.basename(d.rstrip("/"))
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            col = "B" if family(tags["Black"]) == "regicide" else "W"
            bname, bm, nopen = ballot_of(tags["Opening"])
            cake_book_end = nopen - 1
            for i in range(nopen, len(moves)):
                if (i % 2 == 0) != (col == "B") and moves[i][1] and is_book("cake", moves[i][1]):
                    cake_book_end = i
            g = dict(run=run, rnd=tags["Round"], col=col, ballot=bname, nopen=nopen, cbe=cake_book_end,
                     moves=[m for m, _ in moves])
            lines.append(f"{len(games)}\t{tags.get('FEN', '')}\t{col}\t{nopen}\t{' '.join(g['moves'])}")
            games.append(g)
    out = subprocess.run([a.pinwalk, a.book, a.egdb], input="\n".join(lines) + "\n", capture_output=True, text=True,
                         check=True).stdout
    pts, others = {}, collections.defaultdict(list)
    for l in out.splitlines():
        f = l.split("\t")
        if f[1] == "ERR":
            sys.exit(f"pinwalk: {l}")
        gi, ply, fen, mv, el, bmv = int(f[0]), int(f[1]), f[2], f[3], f[4] == "1", f[5]
        g = games[gi]
        if not el or bmv != mv:
            continue
        dist = ply - g["cbe"]
        cat = 2 if 10 <= dist <= 25 else (3 if dist < 10 else 4)
        if fen in pts:
            others[fen].append(f"{g['run']}:{g['rnd']}")
            continue
        pts[fen] = (cat, dist, g["ballot"], g["col"], g["run"], g["rnd"], ply, fen, mv)
    with open(a.out, "w") as fo:
        fo.write("id\tcategory\tdistance\tballot\tcolour\trun\tround\tply\tfen\tpinned\tother_lines\n")
        for i, (fen, p) in enumerate(sorted(pts.items(), key=lambda kv: (kv[1][0], kv[1][2], kv[1][3], kv[1][6]))):
            fo.write(f"P{i + 1}\t" + "\t".join(map(str, p)) + f"\t{','.join(others[fen])}\n")
    c = collections.Counter(p[0] for p in pts.values())
    print(f"{len(games)} games, {len(pts)} distinct points: category 2: {c[2]}, 3: {c[3]}, 4: {c[4]}")


if __name__ == "__main__":
    main()
