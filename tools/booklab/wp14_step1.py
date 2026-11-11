#!/usr/bin/env python3
"""WP14 step 1 read-out (lead 2026-11-11 13:30): where each near-miss game turns.

  wp14_step1.py --contexts ctx.tsv --scores step1.out [--margin 30] [--md OUT] [--candidates cand.tsv]
Per context (ordered by the game's lowest score): for every own position from the end of the ballot to the lowest-score
ply, our engine's 100M root (best move and score, history replayed, book off, 8-piece database) and the 100M score of
the move played (book or searched). The turning point is the latest own BOOK move where the root prefers another
move by MARGIN or more; if there is none, the largest drop of the root score between two consecutive own positions
(the own move and the opponent's reply in between), with our reply options there to be scored next. cand.tsv: the
candidate positions for step 2 (task id, context, ply, kind, played move, the root's best move).
"""
import argparse
import collections
import csv


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--contexts", required=True)
    ap.add_argument("--scores", required=True)
    ap.add_argument("--margin", type=int, default=30)
    ap.add_argument("--md")
    ap.add_argument("--candidates")
    a = ap.parse_args()
    sc = collections.defaultdict(dict)  # task -> label -> (best, score, depth, partial)
    for l in open(a.scores):
        f = l.rstrip("\n").split("\t")
        if len(f) >= 7 and f[1] != "ERR":
            sc[f[0]][f[1]] = (f[2], None if f[3] == "none" else int(f[3]), int(f[4]), f[6] == "1")
        elif len(f) > 1 and f[1] == "ERR":
            raise SystemExit(f"replay error: {l}")
    out, cands = [], []
    for c in csv.DictReader(open(a.contexts), delimiter="\t"):
        rows, turn, pos = [], None, []
        for t in c["tasks"].split():
            tid, ply, mv, kind = t.split(":")
            s = sc.get(tid, {})
            root, played = s.get("root"), s.get(mv) or (s.get("root") if s.get("root") and s["root"][0] == mv else None)
            if root is None:
                rows.append(f"| {ply} | {kind} | {mv} | (not scored yet) | | |")
                continue
            pscore = played[1] if played else None
            diff = (root[1] - pscore) if (root[1] is not None and pscore is not None and root[0] != mv) else 0
            pos.append((int(ply), kind, mv, root, pscore, diff, tid))
            rows.append(f"| {ply} | {kind} | {mv} | {pscore} | {root[0]} {root[1]} (d{root[2]}) | {diff:+d} |")
        book_turns = [p for p in pos if p[1] == "book" and p[5] >= a.margin]
        if book_turns:
            p = book_turns[-1]
            turn = f"latest own book move the 100M root rejects by {p[5]}: ply {p[0]} {p[2]} ({p[4]}), root {p[3][0]} ({p[3][1]})"
            cands.append([p[6], c["context"], str(p[0]), "book", p[2], p[3][0]])
        elif len(pos) >= 2:
            drops = [(pos[i][3][1] - pos[i + 1][3][1], pos[i], pos[i + 1]) for i in range(len(pos) - 1)
                     if pos[i][3][1] is not None and pos[i + 1][3][1] is not None]
            if drops:
                d, p0, p1 = max(drops, key=lambda x: x[0])
                turn = (f"no own book move rejected by {a.margin}+; largest drop {d} between ply {p0[0]} ({p0[3][1]}, our "
                        f"{p0[2]}) and ply {p1[0]} ({p1[3][1]}): the opponent's ply {p0[0] + 1}; our options at ply {p1[0]} to score")
                cands.append([p1[6], c["context"], str(p1[0]), "after-drop", p1[2], p1[3][0]])
        out += [f"### {c['context']}: lowest {c['lowest_score']} at ply {c['lowest_ply']}, book to ply "
                f"{int(c['first_searched_ply']) - 2}", "", f"**Turn:** {turn or '—'}", "",
                "| ply | own move | played | its 100M score | 100M root (best, score, depth) | root − played |",
                "|---|---|---|---|---|---|"] + rows + [""]
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")
    if a.candidates:
        with open(a.candidates, "w") as fo:
            fo.write("task\tcontext\tply\tkind\tplayed\troot_best\n")
            for r in cands:
                fo.write("\t".join(r) + "\n")


if __name__ == "__main__":
    main()
