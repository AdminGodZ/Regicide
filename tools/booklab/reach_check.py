#!/usr/bin/env python3
"""Reachability check of every decision under the book being built (hand-over point 4).

  reach_check.py --decisions decisions.txt --book book.bin --reach booklab_reach --db theory.sqlite
                 [--slack 2] [--limit 300000] [--witnesses 30]

Each decision's ballot (ACF number) and ply come from its evidence text. Two steps:
  1. witnesses: games in the theory table that start from an ACF ballot (the decision's own first; defence trees
     include transpositions from other ballots) and reach the decision's position are
     replayed against the book (wherever our side is to move and the book has a move, the game must play it). One
     consistent game proves the decision reachable.
  2. the rest: booklab_reach searches from the ballot (our side: the book's move where it has one, else any legal
     move; the opponent: any legal move) up to the decision's ply + --slack: "unreachable" is a proof, "unknown" means
     the node limit was hit.
Prints every decision that is not proved reachable, and a summary; exit status 1 if any decision is unreachable.
"""
import argparse
import collections
import os
import re
import sqlite3
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import theory  # noqa: E402


def acf_moves():
    theory.acf_name("")  # fills theory._ACF: moves -> name
    return {int(re.match(r"ACF #(\d+)", n).group(1)): mv for mv, n in theory._ACF.items()}


def run(reach, book, limit, queries, egdb=""):
    out = subprocess.run([reach, book, str(limit)] + ([egdb] if egdb else []), input="\n".join(queries) + "\n", capture_output=True, text=True,
                         check=True).stdout
    return [l.split("\t") for l in out.splitlines()]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--decisions", required=True)
    ap.add_argument("--book", required=True)
    ap.add_argument("--reach", required=True)
    ap.add_argument("--db", required=True)
    ap.add_argument("--slack", type=int, default=2)
    ap.add_argument("--limit", type=int, default=300000)
    ap.add_argument("--witnesses", type=int, default=30)
    ap.add_argument("--egdb", default="", help="endgame database: database-decided roots skip the book, as in Engine::go")
    a = ap.parse_args()
    deck = acf_moves()
    db = sqlite3.connect(a.db)
    dec, skipped = {}, []
    for n, line in enumerate(open(a.decisions), 1):
        if line.startswith("#") or line.count(" | ") < 3:
            continue
        fen, mv, kind, ev = [x.strip() for x in line.split(" | ", 3)]
        b = re.search(r"ACF #(\d+)", ev)
        p = re.search(r"\bply (\d+)", ev)
        if not b or not p or int(b.group(1)) not in deck:
            skipped.append(n)
            continue
        dec[n] = dict(fen=fen, mv=mv, ev=ev, ballot=deck[int(b.group(1))], ply=int(p.group(1)))
    # 1. witnesses
    wq = []
    for n, d in dec.items():
        k = db.execute("SELECT key FROM positions WHERE fen=?", (d["fen"],)).fetchone()
        if not k:
            continue
        # witness games from any ACF ballot (defence trees include transpositions from other ballots), own ballot first
        rows = db.execute("SELECT p.gid, p.ply, g.nopen, g.opening FROM plies p JOIN games g USING(gid) WHERE p.key=? "
                          "AND g.fen='' AND g.nopen=3 ORDER BY (g.opening LIKE ?) DESC LIMIT ?",
                          (k[0], f"%: {d['ballot']}", a.witnesses)).fetchall()
        for gid, ply, nopen, opening in rows:
            mv = [m for (m,) in db.execute("SELECT move FROM plies WHERE gid=? AND ply>=? AND ply<? ORDER BY ply",
                                           (gid, nopen, ply))]
            ballot = opening.rsplit(": ", 1)[1].strip()
            wq.append(f"{n}:{gid}\t{ballot}\t{d['fen']}\t0\t{' '.join(mv) if mv else '-'}")
    proved = {}
    why = collections.defaultdict(list)
    for f in run(a.reach, a.book, a.limit, wq, a.egdb) if wq else []:
        n = int(f[0].split(":")[0])
        if f[1] == "reachable":
            proved[n] = "witness game " + f[0].split(":")[1]
        else:
            why[n].append(f[3])
    # 2. search for the rest
    sq = [f"{n}\t{d['ballot']}\t{d['fen']}\t{d['ply'] + a.slack}" for n, d in dec.items() if n not in proved]
    res = {}
    for f in run(a.reach, a.book, a.limit, sq, a.egdb) if sq else []:
        res[int(f[0])] = f
        if f[1] == "reachable":
            proved[int(f[0])] = "search"
    bad = sorted(n for n in dec if n not in proved)
    for n in bad:
        d, f = dec[n], res.get(n, ["", "?", "0"])
        w = f"; witnesses failed: {why[n][0]}" if why[n] else ""
        print(f"line {n}: {f[1]} ({f[2]} nodes{w}): {d['fen']} | {d['mv']} | {d['ev'][:110]}")
    c = collections.Counter(res[n][1] for n in bad if n in res)
    print(f"reachability: {len(dec)} decisions checked: {len(proved)} reachable "
          f"({sum(1 for v in proved.values() if v.startswith('witness'))} by a witness game), "
          f"{c['unreachable']} unreachable, {c['unknown']} unknown (node limit {a.limit})"
          + (f"; {len(skipped)} without ballot/ply in the evidence (lines {skipped[:10]})" if skipped else ""))
    sys.exit(1 if c["unreachable"] else 0)


if __name__ == "__main__":
    main()
