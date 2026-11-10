#!/usr/bin/env python3
"""Hand-over check: (1) how often each removed decision's position occurred with Regicide to move in the given runs,
(2) whether two books play the same move in every position with Regicide to move in those runs.

  book_equiv.py --runs DIR... --removed removed_decisions.txt --replay booklab_replay --cmp booklab_bookcmp
                [--book-a r8.bin --book-b candidate.bin] --out report.md
"""
import argparse
import collections
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--removed", required=True)
    ap.add_argument("--replay", required=True)
    ap.add_argument("--cmp", required=True)
    ap.add_argument("--book-a")
    ap.add_argument("--book-b")
    ap.add_argument("--out", required=True)
    ap.add_argument("--dump", help="TSV of every compared position with both books' moves")
    a = ap.parse_args()
    lines, reg_col, ngames = [], {}, 0
    for d in a.runs:
        f = os.path.join(d, "games.pdn")
        if not os.path.exists(f):
            continue
        for tags, moves in parse_pdn(open(f, errors="replace").read()):
            col = "B" if family(tags.get("Black", "")) == "regicide" else (
                "W" if family(tags.get("White", "")) == "regicide" else None)
            if col is None:
                continue
            gid = f"g{ngames}"
            ngames += 1
            reg_col[gid] = col
            lines.append(f"{gid}\t{tags.get('FEN', '')}\t{' '.join(m for m, _ in moves)}")
    out = subprocess.run([a.replay], input="\n".join(lines) + "\n", capture_output=True, text=True, check=True).stdout
    fen_of, occ = {}, collections.Counter()
    for l in out.splitlines():
        p = l.split("\t")
        if p[0] == "F":
            fen_of[p[1]] = p[2]
    for l in out.splitlines():
        p = l.split("\t")
        if p[0] == "P":
            fen = fen_of[p[3]]
            if fen[0].upper() == reg_col[p[1]]:  # Regicide to move
                occ[fen] += 1
    removed = [l.rstrip("\n").split(" | ") for l in open(a.removed) if not l.startswith("#") and l.strip()]
    md = [f"# Book hand-over check: {ngames} games, {len(occ)} distinct positions with Regicide to move", "",
          f"Runs: {', '.join(os.path.basename(d.rstrip('/')) for d in a.runs)}", ""]
    hit = [(r, occ[r[0]]) for r in removed if occ[r[0]]]
    md.append(f"**Removed decisions whose position occurred with Regicide to move: {len(hit)} of {len(removed)}.**")
    for r, n in hit:
        md.append(f"- {n}x `{r[0]}` | {r[1]} | {r[3][:100]}")
    md.append("")
    if a.book_a and a.book_b:
        res = subprocess.run([a.cmp, a.book_a, a.book_b, "all"], input="\n".join(occ) + "\n", capture_output=True,
                             text=True, check=True).stdout.splitlines()
        if a.dump:  # every position: FEN, occurrences, move of A, move of B, same/DIFFER
            with open(a.dump, "w") as fo:
                fo.write("fen\toccurrences\tmove_a\tmove_b\tverdict\n")
                for l in res[:-1]:
                    f = l.split("\t")
                    fo.write(f"{f[0]}\t{occ[f[0]]}\t{f[1]}\t{f[2]}\t{f[3]}\n")
        md.append(f"**{os.path.basename(a.book_a)} vs {os.path.basename(a.book_b)}: {res[-1]}** (each position with "
                  f"Regicide to move in these games probed in both books).")
        for l in res[:-1]:
            f = l.split("\t")
            if f[3] == "DIFFER":
                md.append(f"- `{f[0]}`: {f[1]} vs {f[2]} ({occ[f[0]]}x)")
        md.append("")
    open(a.out, "w").write("\n".join(md) + "\n")
    print("\n".join(md[:12]))


if __name__ == "__main__":
    main()
