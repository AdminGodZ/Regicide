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
import csv
import os
import re
import sqlite3
import subprocess
import sys

TOP = ("cake", "kingsrow", "guinn")


_ACF = None


def acf_name(moves):
    """ACF deck name ('ACF #7', 'ACF #153 Black Widow') for a ballot move string, from tools/match/openings.cpp."""
    global _ACF
    if _ACF is None:
        _ACF = {}
        src = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "match", "openings.cpp")
        if not os.path.exists(src):
            src = os.path.expanduser("~/regicide-booklab/tools/match/openings.cpp")
        for m in re.finditer(r'\{(\d+), "([^"]+)", \'(.)\', (?:"([^"]+)"|nullptr)\}', open(src).read()):
            _ACF[m.group(2)] = f"ACF #{m.group(1)}" + (f" {m.group(4)}" if m.group(4) else "")
    return _ACF.get(" ".join(moves.split()), moves)


def ballot_of(opening_tag):
    """(name, moves, nplies) of a match-runner Opening tag ('ACF #7: 9-13 22-18 10-15' or 'file:3: 9-13 ...')."""
    if ": " not in opening_tag:
        return opening_tag, "", 0
    mv = opening_tag.rsplit(": ", 1)[1].strip()
    return acf_name(mv), mv, len(mv.split())


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
            ballot, _, nopen = ballot_of(op)
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


def side_at(fen, ply):
    first = "B" if not fen or fen.upper().startswith("B") else "W"
    return first if ply % 2 == 0 else ("W" if first == "B" else "B")


def pieces(fen):
    n = 0
    for part in fen.split(":")[1:]:
        part = part.strip()[1:]
        n += len([x for x in part.split(",") if x.strip()])
    return n


def lost_for(result, col):
    return (result == "B" and col == "W") or (result == "W" and col == "B")


def repair(args):
    """WP5a: first position of a lost game where our move is not one of the opponent's own non-losing moves there,
    then the opponent's defence from that position (all its non-lost games), as force decisions."""
    db = sqlite3.connect(args.db)
    gid, src, black, white, bfam, wfam, result, opening, ballot, nopen, fen, nplies, term = db.execute(
        "SELECT * FROM games WHERE gid=?", (args.gid,)).fetchone()
    col = "B" if bfam == args.fam else "W"
    opp = args.opp or (wfam if col == "B" else bfam)
    fens = {}

    def fen_of(key):
        if key not in fens:
            fens[key] = db.execute("SELECT fen FROM positions WHERE key=?", (key,)).fetchone()[0]
        return fens[key]

    def opp_rows(key):
        return db.execute("SELECT move, n, w, d, l FROM theory WHERE key=? AND fam=? ORDER BY n DESC",
                          (key, opp)).fetchall()

    print(f"# game {gid} {src}: {ballot} ({opening}), {args.fam} as {col}, result {result}, opponent {opp}")
    path, dev = [], None
    for ply, key, mv in db.execute("SELECT ply, key, move FROM plies WHERE gid=? ORDER BY ply", (gid,)).fetchall():
        if ply < nopen or side_at(fen, ply) != col:
            continue
        rows = opp_rows(key)
        ok = [r for r in rows if r[4] == 0]
        if ok and mv not in [r[0] for r in ok]:
            dev = (ply, key, mv, rows)
            break
        path.append((ply, key, mv, rows))
        if ply - nopen > args.max_search:
            break
    for ply, key, mv, rows in path:
        print(f"#   ply {ply} {mv}: {opp} " + (", ".join(f"{r[0]} {r[1]}x {r[2]}/{r[3]}/{r[4]}" for r in rows) or "-"))
    if dev is None:
        print(f"# no deviation from {opp}'s non-losing moves within {args.max_search} plies after the ballot")
        return
    ply0, key0, mv0, rows0 = dev
    print(f"# DEVIATION ply {ply0} ({ply0 - nopen} after the ballot): {args.fam} played {mv0}; {opp}: "
          + ", ".join(f"{r[0]} {r[1]}x w/d/l {r[2]}/{r[3]}/{r[4]}" for r in rows0))
    print(f"#   FEN {fen_of(key0)}")
    # The opponent's defence from the deviation position, over every game where it defended it without losing.
    counts, first_ply, games_used = {}, {}, set()
    for g2, p2, fen2, res2 in db.execute(
            "SELECT p.gid, p.ply, g.fen, g.result FROM plies p JOIN games g USING(gid) WHERE p.key=? AND p.fam=?",
            (key0, opp)).fetchall():
        if side_at(fen2, p2) != col or lost_for(res2, col):
            continue
        games_used.add(g2)
        for p3, k3, m3, f3 in db.execute(
                "SELECT ply, key, move, fam FROM plies WHERE gid=? AND ply>=? AND ply<? ORDER BY ply",
                (g2, p2, p2 + args.follow)).fetchall():
            if (p3 - p2) % 2:
                continue
            if f3 != opp or pieces(fen_of(k3)) <= args.min_pieces:
                break
            counts.setdefault(k3, {}).setdefault(m3, 0)
            counts[k3][m3] += 1
            first_ply[k3] = min(first_ply.get(k3, 999), p3)
    print(f"# {opp}'s defence: {len(games_used)} non-lost games from the deviation position, "
          f"{len(counts)} positions of our side within {args.follow} plies (stop at <= {args.min_pieces} pieces)")
    out = []
    if args.pin_path:
        for ply, key, mv, rows in path:
            r = [x for x in rows if x[0] == mv]
            if r:
                out.append((ply, key, mv, f"{opp} played the same move {r[0][1]}x (w/d/l {r[0][2]}/{r[0][3]}/"
                                          f"{r[0][4]}) on the path to the deviation"))
    for k, mvs in sorted(counts.items(), key=lambda kv: first_ply[kv[0]]):
        best = max(mvs.items(), key=lambda kv: kv[1])
        th = {r[0]: r for r in opp_rows(k)}
        t = th.get(best[0])
        alts = ", ".join(f"{m} {c}x" for m, c in mvs.items() if m != best[0])
        ev = (f"{opp} defence: {best[0]} in {best[1]} of {sum(mvs.values())} non-lost {opp} games from here"
              + (f" (theory {t[1]}x w/d/l {t[2]}/{t[3]}/{t[4]})" if t else "") + (f"; also {alts}" if alts else ""))
        if k == key0:
            ev += f"; {args.fam} played {mv0} here and lost (gid {gid}, {os.path.basename(os.path.dirname(src))})"
        out.append((first_ply[k], k, best[0], ev))
    for ply, k, mv, ev in out:
        print(f"{fen_of(k)} | {mv} | force | {ballot} {args.fam} {col} ply {ply}: {ev}")


def export(args):
    """Opening part of the theory table as CSV: positions up to --max-ply (from the start) where a top engine moved,
    one row per (position, family, move)."""
    db = sqlite3.connect(args.db)
    keys = {k for (k,) in db.execute("SELECT DISTINCT key FROM plies WHERE ply <= ? AND fam IN "
                                     "('cake','kingsrow','guinn')", (args.max_ply,))}
    w = csv.writer(sys.stdout)
    w.writerow(["fen", "family", "move", "n", "n_instant", "n_book", "w", "d", "l"])
    rows = 0
    for k in sorted(keys):
        fen = db.execute("SELECT fen FROM positions WHERE key=?", (k,)).fetchone()[0]
        for fam, mv, n, ni, nb, ww, dd, ll in db.execute(
                "SELECT fam, move, n, n_instant, n_book, w, d, l FROM theory WHERE key=? ORDER BY fam, n DESC", (k,)):
            w.writerow([fen, fam, mv, n, ni, nb, ww, dd, ll])
            rows += 1
    print(f"{len(keys)} positions, {rows} rows", file=sys.stderr)


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
    r = sub.add_parser("repair")
    r.add_argument("--db", required=True)
    r.add_argument("--gid", type=int, required=True)
    r.add_argument("--fam", default="regicide")
    r.add_argument("--opp", help="family whose defence to follow (default: the opponent in the game)")
    r.add_argument("--max-search", type=int, default=60, help="plies after the ballot to look for the deviation")
    r.add_argument("--follow", type=int, default=40, help="plies of the opponent's defence to force")
    r.add_argument("--min-pieces", type=int, default=8, help="stop forcing at this many pieces (egdb)")
    r.add_argument("--pin-path", action="store_true", help="also force our (matching) moves before the deviation")
    e = sub.add_parser("export")
    e.add_argument("--db", required=True)
    e.add_argument("--max-ply", type=int, default=24)
    args = ap.parse_args()
    {"build": build, "alts": alts, "repair": repair, "export": export}[args.cmd](args)


if __name__ == "__main__":
    main()
