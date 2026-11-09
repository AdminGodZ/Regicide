#!/usr/bin/env python3
"""booklab theory table (WP2).

Parses every games.pdn given (match-runner PDNs with or without per-move comments), replays the games with the
engine's own rules (booklab_replay, built from tools/booklab/replay.cpp) and stores them in an SQLite database:

  games(gid, src, black, white, bfam, wfam, result, opening, ballot, nopen, fen, nplies, term)
      result: 'B' (Black won), 'W' (White won), 'D'; nopen: plies of the opening (ballot) before the engines moved
  plies(gid, ply, key, move, fam, t, book, score)
      fam: mover's engine family (cake, kingsrow, guinn, regicide, simplech) or 'ballot' for opening plies
      t: seconds from the PDN comment (NULL if none); book: 1 if the comment marks a book move; score: the mover's
      reported score (Regicide 'score N', KingsRow-in-CB 'value=N', GuiNN 'Eval: N', Cake book 'v N'), else NULL
  positions(key, fen)
  theory(key, fam, move, n, n_instant, n_book, w, d, l)      w/d/l from the mover's side, one row per family
      (top engines individually; regicide kept separate; ballot plies excluded)

  theory.py build --db theory.sqlite --replay PATH [--list FILE | PDN ...]
  theory.py alts --db theory.sqlite --gid N [--fam regicide]   alternatives by the top engines at each of the
      mover's positions in game N (see WP5a)
"""
import argparse
import os
import re
import sqlite3
import subprocess
import sys

TOP = ("cake", "kingsrow", "guinn")


def family(name):
    n = name.lower()
    for f in ("cake", "kingsrow", "guinn", "simplech"):
        if f in n:
            return f
    if "simple" in n:
        return "simplech"
    return "regicide"


TAG = re.compile(r'^\[(\w+)\s+"(.*)"\]\s*$')
MOVE = re.compile(r"^\d+(?:[-x]\d+)+$")
TIME = re.compile(r"^\s*(\d+\.\d+)s")
SCORES = [re.compile(r"\bscore (-?\d+)"), re.compile(r"value=(-?\d+)"), re.compile(r"Eval:\s*(-?\d+)"),
          re.compile(r"book move: \S+ v (-?\d+)"), re.compile(r"^\S+s book \S+ v(-?\d+)")]


def is_book(fam, c):
    if c is None:
        return 0
    if fam == "regicide":
        return int(bool(re.search(r"depth 0/0 score -?\d+ nodes 0 ", c)))
    if fam == "cake":
        return int("book" in c)
    if fam == "kingsrow":
        return int(bool(re.search(r"\(-?\d+/\d+\)[*?!]", c)))
    if fam == "guinn":
        return int("Depth: 0/0" in c and "Nodes: 0 n" in c)
    return 0


def parse_pdn(text):
    """Yields (tags, [(move, comment|None)])."""
    tags, body = {}, []
    in_body = False

    def flush():
        if not tags:
            return None
        return tags, tokenize(" ".join(body))

    for line in text.splitlines():
        m = TAG.match(line.strip())
        if m:
            if in_body:
                g = flush()
                if g:
                    yield g
                tags, body, in_body = {}, [], False
            tags[m.group(1)] = m.group(2)
        elif line.strip():
            in_body = True
            body.append(line.strip())
    g = flush()
    if g:
        yield g


def tokenize(s):
    out = []
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c == "{":
            j = s.find("}", i)
            j = n if j < 0 else j
            com = s[i + 1:j].strip()
            if out and out[-1][1] is None:
                out[-1] = (out[-1][0], com)
            i = j + 1
        elif c.isspace():
            i += 1
        else:
            j = i
            while j < n and not s[j].isspace() and s[j] != "{":
                j += 1
            tok = s[i:j]
            i = j
            if tok.endswith("."):
                continue
            if re.match(r"^\d+\.", tok):
                tok = tok.split(".", 1)[1]
                if not tok:
                    continue
            if MOVE.match(tok) and tok not in ("1-0", "0-1", "2-0", "0-2", "1-1"):
                out.append((tok, None))
    return out


def score_of(c):
    if c is None:
        return None
    for r in SCORES:
        m = r.search(c)
        if m:
            return int(m.group(1))
    return None


def build(args):
    files = list(args.pdn)
    if args.list:
        files += [l.split()[1] if len(l.split()) > 1 else l.strip() for l in open(args.list) if l.strip()]
    files = [f for f in dict.fromkeys(files) if os.path.exists(f)]
    if os.path.exists(args.db):
        os.remove(args.db)
    db = sqlite3.connect(args.db)
    db.executescript("""
      PRAGMA journal_mode=OFF; PRAGMA synchronous=OFF;
      CREATE TABLE games(gid INTEGER PRIMARY KEY, src TEXT, black TEXT, white TEXT, bfam TEXT, wfam TEXT,
        result TEXT, opening TEXT, ballot TEXT, nopen INT, fen TEXT, nplies INT, term TEXT);
      CREATE TABLE plies(gid INT, ply INT, key TEXT, move TEXT, fam TEXT, t REAL, book INT, score INT);
      CREATE TABLE positions(key TEXT PRIMARY KEY, fen TEXT);
    """)
    tmp_in, tmp_out = args.db + ".replay_in", args.db + ".replay_out"
    rin = open(tmp_in, "w")
    meta = {}  # gid -> [(fam, t, book, score)] per ply
    gid = 0
    ngames = 0
    for f in files:
        src = os.path.relpath(f, os.path.expanduser("~"))
        try:
            text = open(f, errors="replace").read()
        except OSError:
            continue
        for tags, moves in parse_pdn(text):
            if not moves:
                continue
            gid += 1
            b, w = tags.get("Black", "?"), tags.get("White", "?")
            res = {"1-0": "B", "0-1": "W", "1/2-1/2": "D"}.get(tags.get("Result", ""), "?")
            op = tags.get("Opening", "")
            ballot = op.rsplit(": ", 1)[0] if ": " in op else op
            nopen = len(op.rsplit(": ", 1)[1].split()) if ": " in op else 0
            fen = tags.get("FEN", "")
            info = []
            bf, wf = family(b), family(w)
            # side to move at ply 0: from FEN, else Black (English checkers starts with Black)
            first = "B" if not fen or fen.upper().startswith("B") else "W"
            for ply, (mv, c) in enumerate(moves):
                side = first if ply % 2 == 0 else ("W" if first == "B" else "B")
                fam = "ballot" if ply < nopen else (bf if side == "B" else wf)
                t = None
                if c is not None:
                    m = TIME.match(c)
                    t = float(m.group(1)) if m else None
                info.append((fam, t, is_book(fam, c), score_of(c)))
            meta[gid] = info
            db.execute("INSERT INTO games VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?)",
                       (gid, src, b, w, bf, wf, res, op, ballot, nopen, fen, len(moves), tags.get("Termination", "")))
            rin.write(f"{gid}\t{fen}\t{' '.join(m for m, _ in moves)}\n")
            ngames += 1
    rin.close()
    with open(tmp_in) as fi, open(tmp_out, "w") as fo:
        subprocess.run([args.replay], stdin=fi, stdout=fo, check=True)
    nplies = npos = nerr = 0
    batch, fbatch = [], []
    with open(tmp_out) as fo:
        for line in fo:
            p = line.rstrip("\n").split("\t")
            if p[0] == "P":
                g, ply = int(p[1]), int(p[2])
                fam, t, bk, sc = meta[g][ply]
                batch.append((g, ply, p[3], p[4], fam, t, bk, sc))
                nplies += 1
                if len(batch) >= 200000:
                    db.executemany("INSERT INTO plies VALUES(?,?,?,?,?,?,?,?)", batch)
                    batch = []
            elif p[0] == "F":
                fbatch.append((p[1], p[2]))
                npos += 1
            else:
                nerr += 1
                if nerr <= 20:
                    print("replay error", p[1:], file=sys.stderr)
    db.executemany("INSERT INTO plies VALUES(?,?,?,?,?,?,?,?)", batch)
    db.executemany("INSERT INTO positions VALUES(?,?)", fbatch)
    os.remove(tmp_in)
    os.remove(tmp_out)
    db.executescript("""
      CREATE INDEX plies_key ON plies(key);
      CREATE INDEX plies_gid ON plies(gid, ply);
      CREATE TABLE theory AS
        SELECT p.key, p.fam, p.move, COUNT(*) n, SUM(p.t IS NOT NULL AND p.t < 0.05) n_instant, SUM(p.book) n_book,
          SUM((g.result='B' AND p.ply % 2 = (CASE WHEN g.fen='' OR upper(substr(g.fen,1,1))='B' THEN 0 ELSE 1 END))
              OR (g.result='W' AND p.ply % 2 = (CASE WHEN g.fen='' OR upper(substr(g.fen,1,1))='B' THEN 1 ELSE 0 END))) w,
          SUM(g.result='D') d,
          SUM((g.result='W' AND p.ply % 2 = (CASE WHEN g.fen='' OR upper(substr(g.fen,1,1))='B' THEN 0 ELSE 1 END))
              OR (g.result='B' AND p.ply % 2 = (CASE WHEN g.fen='' OR upper(substr(g.fen,1,1))='B' THEN 1 ELSE 0 END))) l
        FROM plies p JOIN games g USING(gid) WHERE p.fam != 'ballot'
        GROUP BY p.key, p.fam, p.move;
      CREATE INDEX theory_key ON theory(key);
    """)
    db.commit()
    print(f"{len(files)} files, {ngames} games, {nplies} plies, {npos} positions, {nerr} replay errors")
    for fam, n, pos in db.execute("SELECT fam, SUM(n), COUNT(DISTINCT key) FROM theory GROUP BY fam"):
        print(f"  {fam}: {n} moves in {pos} positions")


def alts(args):
    db = sqlite3.connect(args.db)
    g = db.execute("SELECT * FROM games WHERE gid=?", (args.gid,)).fetchone()
    print("game", g)
    for ply, key, mv, fam, t, sc in db.execute(
            "SELECT ply, key, move, fam, t, score FROM plies WHERE gid=? ORDER BY ply", (args.gid,)):
        if fam != args.fam:
            continue
        rows = db.execute("SELECT fam, move, n, n_instant, w, d, l FROM theory WHERE key=? AND fam IN "
                          "('cake','kingsrow','guinn') ORDER BY n DESC", (key,)).fetchall()
        other = [r for r in rows if r[1] != mv]
        flag = " <-- top engines played other moves here" if other else ""
        print(f"ply {ply} {mv} t={t} score={sc}{flag}")
        for r in rows:
            print(f"    {r[0]:9s} {r[1]:7s} n={r[2]} instant={r[3]} w/d/l={r[4]}/{r[5]}/{r[6]}")
        if ply > args.max_ply:
            break


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="cmd", required=True)
    b = sub.add_parser("build")
    b.add_argument("--db", required=True)
    b.add_argument("--replay", required=True)
    b.add_argument("--list")
    b.add_argument("pdn", nargs="*")
    a = sub.add_parser("alts")
    a.add_argument("--db", required=True)
    a.add_argument("--gid", type=int, required=True)
    a.add_argument("--fam", default="regicide")
    a.add_argument("--max-ply", type=int, default=60)
    args = ap.parse_args()
    {"build": build, "alts": alts}[args.cmd](args)


if __name__ == "__main__":
    main()
