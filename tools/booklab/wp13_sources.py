#!/usr/bin/env python3
"""WP13: the three sources at each of Regicide's positions on the lines lost against KingsRow in book mode 3.

  wp13_sources.py --fens fens.tsv --scores score20m.tsv --kr kr_defence.tsv --db theory_w13.sqlite [--margin 15]
fens.tsv: ID (#ballot:ply, one-based ply of our move) <TAB> FEN; score20m.tsv: booklab_multiscore output (20 M
nodes, book off, cleared hash per move); kr_defence.tsv: KingsRow mode 3's answer at our positions (kr_defence.py);
theory_w13.sqlite: theory table built only from games against KingsRow mode 3.
Per position: our move in the lost game, Cake's moves there as Black with their w/d/l (non-lost = d + w from the
mover's side), KingsRow's mode-3 book/search move, our search's best move and score (and our move's score). The
repair point is the earliest position where our move is neither a non-lost Cake move nor KingsRow's move (where they
have one); the proposed move is one at least two sources agree on (Cake non-lost, KingsRow, our search's best or a
move within MARGIN of it). Rule from the lead, 2026-11-11.
"""
import argparse
import collections
import sqlite3


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--fens", required=True)
    ap.add_argument("--scores", required=True)
    ap.add_argument("--kr", required=True)
    ap.add_argument("--db", required=True)
    ap.add_argument("--games", required=True, help="points.tsv of wp13_score.sh: ballot <TAB> move list")
    ap.add_argument("--margin", type=int, default=15)
    a = ap.parse_args()
    fens = [l.rstrip("\n").split("\t") for l in open(a.fens)]
    sc = collections.defaultdict(dict)
    for l in open(a.scores):
        f = l.rstrip("\n").split("\t")
        if f[1] != "ERR":
            sc[f[0]][f[1]] = (None if f[2] == "none" else int(f[2]), int(f[3]))
    moves = {}
    for l in open(a.games):
        b, mv = l.rstrip("\n").split("\t")
        moves[b] = mv.split()
    kr = {}
    for l in open(a.kr):
        if l.startswith("#") or l.startswith("game\t"):
            continue
        f = l.rstrip("\n").split("\t")
        b = f[0].split("ACF ")[1].split()[0]  # '#60'
        kr[(b, int(f[1]))] = (f[3], f[5][:60])
    db = sqlite3.connect(a.db)
    key_of = {fen: key for key, fen in db.execute("SELECT key, fen FROM positions")}
    out_lines, repairs = [], {}
    for pid, fen in fens:
        b, ply = pid.split(":")
        ply = int(ply)
        ours = moves[b][ply - 1]
        cake = {}
        k = key_of.get(fen)
        if k is not None:
            for mv, n, w, d, l in db.execute("SELECT move, n, w, d, l FROM theory WHERE key=? AND fam='cake'", (k,)):
                cake[mv] = (n, w, d, l)
        cake_ok = {m for m, (n, w, d, l) in cake.items() if l < n}
        krm = kr.get((b, ply), (None, ""))[0]
        if b == "#9":
            krm = krm  # kr_defence has #9's own game
        s = sc.get(pid, {})
        best = max((m for m in s if s[m][0] is not None), key=lambda m: s[m][0], default=None)
        near = {m for m in s if s[m][0] is not None and best and s[m][0] >= s[best][0] - a.margin}
        leaves = (cake or krm) and ours not in cake_ok and ours != krm
        votes = collections.Counter()
        for m in cake_ok:
            votes[m] += 1
        if krm:
            votes[krm] += 1
        for m in near:
            votes[m] += 1
        agreed = sorted((m for m, v in votes.items() if v >= 2), key=lambda m: (-votes[m], -(s.get(m, (-99999,))[0] or -99999)))
        out_lines.append(f"{pid}\tours {ours} ({s.get(ours, ('-',))[0]})\tcake {','.join(f'{m}:{cake[m][2]}d{cake[m][3]}l' for m in cake) or '-'}"
                         f"\tKR {krm or '-'}\tsearch {best} ({s[best][0] if best else '-'}, d{s[best][1] if best else '-'})"
                         f"\t{'LEAVES' if leaves else ''}\t{'agreed: ' + ','.join(agreed) if agreed else ''}")
        if leaves and b not in repairs:
            repairs[b] = (pid, fen, ours, agreed, sorted(cake_ok), krm, best)
    print("\n".join(out_lines))
    print("\nRepair points (earliest position where our move leaves Cake's non-lost moves and KingsRow's move):")
    for b, (pid, fen, ours, agreed, cake_ok, krm, best) in repairs.items():
        print(f"{b}: {pid} {fen} ours {ours}; Cake {cake_ok}, KingsRow {krm}, search {best}; "
              f"{'PROPOSED ' + agreed[0] if agreed else 'NO TWO SOURCES AGREE: lead decision'}"
              f"{(' (also agreed: ' + ','.join(agreed[1:]) + ')') if len(agreed) > 1 else ''}")


if __name__ == "__main__":
    main()
