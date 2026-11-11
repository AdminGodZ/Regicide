#!/usr/bin/env python3
"""WP12 gates (PROTOCOL.md on master): game analysis for a one-key candidate and the stage-2 and stage-3 verdicts.

Shared by wp12_discover.py (stage 1), wp12_confirm.py (stages 2 and 3, runner and evaluator) and the tests
(tests_wp12_gates.py, made-up records only).

Consulted (review 2026-11-11, finding 3): a game consulted the changed key when, at the key's position with Regicide
to move, (a) the book is consulted there (booklab_pinwalk eligible = 1: no reversible plies, root not decided by the
database), (b) the walked book (the candidate book for a candidate game, the unchanged book for a control game)
plays the expected move there (pinwalk's book move), (c) the game played that move, and (d) the record shows it was a
book move: Regicide's PDN comment at that ply reads "depth 0/0 ... nodes 0" (a searched move that happens to equal
the book move is not a consultation).

On the board (finding 4): a game counts only if its termination is one of the runner's on-board texts (tools/match/
main.cpp on master): "<colour> has no legal moves", "threefold repetition", "<N>-move rule ...", "endgame database
adjudication", "adjudicated draw after ...", "draw claimed by ...". Everything else ("crashed", "did not start a new
game", "timed out", "illegal move", "protocol error", "lost on time", "interrupted", empty) is off the board.
"""
import collections
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

ON_BOARD = (re.compile(r"^(white|black) has no legal moves$"), re.compile(r"^threefold repetition$"),
            re.compile(r"^\d+-move rule"), re.compile(r"^endgame database adjudication$"),
            re.compile(r"^adjudicated draw after \d+ plies"), re.compile(r"^draw claimed by "))
BOOK_COMMENT = re.compile(r"\bdepth 0/0\b.*\bnodes 0\b")


def on_board(termination):
    t = (termination or "").strip().lower()
    return any(p.search(t) for p in ON_BOARD)


def result_of(tags, col):
    return {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(
        tags.get("Result", "").strip())


def load_games(pdn_path):
    """Games of one run with exactly one Regicide side: tags, moves [(move, comment)], colour, result."""
    out = []
    for tags, moves in parse_pdn(open(pdn_path, errors="replace").read()):
        rb, rw = family(tags.get("Black", "")) == "regicide", family(tags.get("White", "")) == "regicide"
        if rb == rw:
            raise ValueError(f"{pdn_path} round {tags.get('Round')}: not exactly one Regicide side")
        col = "B" if rb else "W"
        out.append(dict(tags=tags, moves=moves, col=col, res=result_of(tags, col),
                        opp=family(tags["White"] if rb else tags["Black"]), ballot=ballot_of(tags["Opening"]),
                        term=tags.get("Termination", "")))
    return out


def pinwalk(pinwalk_bin, book, egdb, games):
    """booklab_pinwalk over games (from their FEN tag if any). Returns {index: [rows]}; a game whose replay fails
    raises (never dropped)."""
    inp = "".join(f"{i}\t{g['tags'].get('FEN', '')}\t{g['col']}\t{g['ballot'][2]}\t{' '.join(m for m, _ in g['moves'])}\n"
                  for i, g in enumerate(games))
    out = subprocess.run([pinwalk_bin, book, egdb], input=inp, capture_output=True, text=True, check=True).stdout
    rows = collections.defaultdict(list)
    for l in out.splitlines():
        f = l.split("\t")
        if f[1] == "ERR":
            raise ValueError(f"pinwalk could not replay game {f[0]}: {' '.join(f[2:])}")
        rows[int(f[0])].append(dict(ply=int(f[1]), fen=f[2], move=f[3], eligible=f[4] == "1", book=f[5],
                                    avoided=set() if len(f) < 7 or f[6] == "-" else set(f[6].split(","))))
    return rows


def key_use(rows, comments, fen, expected):
    """(reached, consulted, ply) at the key position fen for one game: rows = its pinwalk rows on the walked book,
    comments = its PDN comments by ply, expected = the move the walked book must play there."""
    reached, consulted, at = 0, 0, None
    for r in rows:
        if r["fen"] != fen:
            continue
        reached = 1
        at = r["ply"] if at is None else at
        c = comments[r["ply"]] if r["ply"] < len(comments) else None
        if r["eligible"] and r["book"] == expected and r["move"] == expected and BOOK_COMMENT.search(c or ""):
            consulted, at = 1, r["ply"]
    return reached, consulted, at


def analyse_run(pdn_path, pinwalk_bin, book, egdb, fen, expected, **extra):
    """One record per game of the run: result, reached, consulted, on the board."""
    gs = load_games(pdn_path)
    pw = pinwalk(pinwalk_bin, book, egdb, gs)
    out = []
    for i, g in enumerate(gs):
        reached, consulted, at = key_use(pw[i], [c for _, c in g["moves"]], fen, expected)
        out.append(dict(extra, result=g["res"] or "?", reached=reached, consulted=consulted, key_ply=at,
                        off_board=not on_board(g["term"]) or g["res"] is None, term=g["term"], opp=g["opp"],
                        ballot=g["ballot"][0], colour=g["col"], round=g["tags"].get("Round", "?"),
                        plies=len(g["moves"])))
    return out


# ---- stage 2: confirmation against Cake (12 candidate games, 12 control games, >= 3 separately started blocks) ----
S2_GAMES, S2_BLOCKS, S2_WINS, S2_MARGIN = 12, 3, 4, 3


def eval_stage2(recs):
    """recs: dicts with arm ('cand' or 'ctrl'), block, result, consulted, off_board. Returns (verdict, reasons,
    counts); verdict is 'keep', 'reject' or 'not counted' (incomplete, or a game off the board)."""
    cand = [r for r in recs if r["arm"] == "cand"]
    ctrl = [r for r in recs if r["arm"] == "ctrl"]
    blocks = {r["block"] for r in cand} & {r["block"] for r in ctrl}
    c = dict(cand_games=len(cand), ctrl_games=len(ctrl), blocks=len(blocks),
             cand_W=sum(r["result"] == "W" for r in cand), cand_D=sum(r["result"] == "D" for r in cand),
             cand_L=sum(r["result"] == "L" for r in cand), exposed=sum(r["consulted"] for r in cand),
             exposed_wins=sum(r["result"] == "W" and r["consulted"] for r in cand),
             ctrl_W=sum(r["result"] == "W" for r in ctrl), ctrl_L=sum(r["result"] == "L" for r in ctrl),
             ctrl_reached=sum(r["consulted"] for r in ctrl))
    off = [r for r in recs if r["off_board"]]
    if off:
        return "not counted", [f"{len(off)} game(s) not ended on the board: {off[0].get('term')!r}"], c
    if len(cand) != S2_GAMES or len(ctrl) != S2_GAMES:
        return "not counted", [f"incomplete: {len(cand)} candidate and {len(ctrl)} control games of {S2_GAMES}"], c
    why = []
    if len(blocks) < S2_BLOCKS:
        why.append(f"{len(blocks)} separately started blocks (at least {S2_BLOCKS})")
    if c["cand_L"]:
        why.append(f"{c['cand_L']} loss(es) with the candidate")
    if c["exposed_wins"] < S2_WINS:
        why.append(f"{c['exposed_wins']} wins in games that consulted the key (at least {S2_WINS})")
    if c["exposed_wins"] - c["ctrl_W"] < S2_MARGIN:
        why.append(f"{c['exposed_wins']} exposed wins against {c['ctrl_W']} control wins (at least {S2_MARGIN} more)")
    return ("reject", why, c) if why else ("keep", [], c)


# ---- stage 3: other opponents and other ways in ----
S3_GAMES, S3_MIN_CONSULTED = 12, 6


def stage3_plan(contexts, nominating):
    """(opponent, context) pairs to play, 12 games each: KingsRow and GuiNN from the nominating context; all three
    opponents from every other recorded context (Cake from the nominating one is stage 2)."""
    plan = [(o, nominating) for o in ("kingsrow", "guinn")]
    for ctx in contexts:
        if ctx != nominating:
            plan += [(o, ctx) for o in ("cake", "kingsrow", "guinn")]
    return plan


def eval_stage3(recs, plan):
    """recs: dicts with opp, context, result, consulted, off_board. Returns (verdict, reasons, per-opponent counts);
    verdict 'reject' (a loss in a game that consulted the key), 'not counted' (a planned pair incomplete, or a game
    off the board) or 'pass'; reasons also list opponents with fewer than 6 consultations (untested against them, not
    safe) and losses in games that did not consult the key (reported, book problem outside the candidate)."""
    per = collections.defaultdict(collections.Counter)
    for r in recs:
        k = per[r["opp"]]
        k["games"] += 1
        k[r["result"]] += 1
        k["consulted"] += r["consulted"]
        k["consulted_L"] += r["consulted"] and r["result"] == "L"
        k["other_L"] += (not r["consulted"]) and r["result"] == "L"
    off = [r for r in recs if r["off_board"]]
    if off:
        return "not counted", [f"{len(off)} game(s) not ended on the board: {off[0].get('term')!r}"], per
    n = collections.Counter((r["opp"], r["context"]) for r in recs)
    short = [f"{o} from {ctx}: {n[(o, ctx)]} of {S3_GAMES}" for o, ctx in plan if n[(o, ctx)] != S3_GAMES]
    if short:
        return "not counted", ["incomplete: " + "; ".join(short)], per
    why = [f"{per[o]['consulted_L']} loss(es) against {o} in games that consulted the key" for o in sorted(per)
           if per[o]["consulted_L"]]
    notes = [f"untested against {o}: the key was consulted in {per[o]['consulted']} games (at least {S3_MIN_CONSULTED})"
             for o in sorted({o for o, _ in plan} | {"cake"}) if per[o]["consulted"] < S3_MIN_CONSULTED and o in per]
    notes += [f"{per[o]['other_L']} loss(es) against {o} in games that did not consult the key (report; not the "
              "candidate's move)" for o in sorted(per) if per[o]["other_L"]]
    return ("reject", why + notes, per) if why else ("pass", notes, per)
