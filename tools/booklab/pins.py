#!/usr/bin/env python3
"""WP11: pin the validated lines of the final validation games.

  pins.py --runs DIR... --book book_r10.bin --pinwalk booklab_pinwalk --egdb DIR --out pins.txt --report pins.md

For every position with Regicide to move in the given games, from the end of the ballot until the position has <= 8
pieces (from there the database and the conversion search play) or the game ends: the move that was played becomes a
forced book move ("pin"), unless
  - the book (r10: existing decisions and old-book moves) already has a move there: the existing move wins; if it
    differs from the played move the case is listed (it happens where the engine skipped the book);
  - the engine would not consult its book there (inside a run of king moves, or a database-decided root): left out
    and counted.
Different moves in the same position across games: the move whose games have the best worst result for us
(win > draw > loss), then the more frequent one, then the one played in run a. Every such conflict is listed.
Evidence on each pin: opponent, ballot, colour, run, round, result, and how many games played it.
"""
import argparse
import collections
import os
import re
import statistics
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of, is_book, TIME  # noqa: E402

RANK = {"W": 2, "D": 1, "L": 0}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", nargs="+", required=True)
    ap.add_argument("--book", required=True)
    ap.add_argument("--pinwalk", required=True)
    ap.add_argument("--egdb", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--report", required=True)
    a = ap.parse_args()
    games, lines, unfinished, not_regicide = [], [], 0, 0
    # Input order is frozen: runs sorted by name, games in file order. It decides the remaining ties (after worst
    # result, frequency and run a, the first move met wins).
    for d in sorted(a.runs):
        run = os.path.basename(d.rstrip("/"))  # bl-f1-cake-a-h1, bl-v13k4-cake-a-h2, bl-t1a-60-cake-H3_0
        # run letter for the tie-break "run a" from the name (-a- / -b-); the opponent from the player tags below
        ra = "a" if "-a-" in run else ("b" if "-b-" in run else "-")
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            # exactly one side Regicide, the other a known opponent (review 2026-11-11: an opponent-only file must
            # not turn the opponent's moves into Regicide pins); anything else is skipped and counted
            # positive matches only: family() maps any unknown name to "regicide", so Regicide is recognised by its
            # name and the opponent must be one of the known engines (review 2026-11-11)
            nb, nw = tags.get("Black", ""), tags.get("White", "")
            rb, rw = nb.strip().lower().startswith("regicide"), nw.strip().lower().startswith("regicide")
            other = nw if rb else nb
            if rb == rw or family(other) not in ("cake", "guinn", "kingsrow"):
                not_regicide += 1
                continue
            col = "B" if rb else "W"
            opp = family(other)
            res = tags.get("Result", "").strip()
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res)
            if r is None:  # unfinished or interrupted: no pins from it
                unfinished += 1
                continue
            bname, bm, nopen = ballot_of(tags["Opening"])
            g = dict(run=run, opp=opp, ra=ra, rnd=tags["Round"], col=col, res=r, ballot=bname, nopen=nopen,
                     moves=[m for m, _ in moves], comments=[c for _, c in moves])
            lines.append(f"{len(games)}\t{tags.get('FEN', '')}\t{col}\t{nopen}\t{' '.join(g['moves'])}")
            games.append(g)
    out = subprocess.run([a.pinwalk, a.book, a.egdb], input="\n".join(lines) + "\n", capture_output=True, text=True,
                         check=True).stdout
    rows = collections.defaultdict(list)  # fen -> [(game, ply, move, eligible, bookmove)]
    per_game = collections.defaultdict(list)
    rejected = []
    for l in out.splitlines():
        f = l.split("\t")
        if f[1] == "ERR":  # the whole game is left out (pinwalk prints nothing else for it)
            rejected.append(l)
            continue
        gi, ply, fen, mv, el, bmv = int(f[0]), int(f[1]), f[2], f[3], f[4] == "1", f[5]
        av = set(f[6].split(",")) if len(f) > 6 and f[6] != "-" else set()  # the base book's avoided moves here
        rows[fen].append((gi, ply, mv, el, bmv, av))
        per_game[gi].append((ply, fen, mv, el, bmv, av))
    # A lost game counts against a move only if it is still playable under the base book (lead 2026-11-11, r11c): if
    # at a later own position the base book plays a different move than the game did (a WP13 decision, or an old-book
    # move), the loss is repaired downstream and is left out of the evidence of every earlier position of that game.
    # A later own move that the base book avoids makes the lost game unplayable just as a forced move does (review
    # 2026-11-11). The avoid list applies at every root that is not database-decided, also inside a run of king moves
    # (only book moves are gated by the book's eligibility; review 2026-11-11 residual), so an avoided own move counts
    # wherever pinwalk reports a position (its rows stop above 8 pieces, so no reported root is database-decided).
    repaired_from = {}  # lost game -> plies of its own positions where the base book plays another move
    for gi, g in enumerate(games):
        if g["res"] == "L":
            repaired_from[gi] = [ply for ply, fen, mv, el, bmv, av in per_game[gi]
                                 if (el and bmv != "-" and bmv != mv) or mv in av]
    def repaired(r):  # r = (game, ply, move, eligible, bookmove)
        return r[0] in repaired_from and any(q > r[1] for q in repaired_from[r[0]])
    repaired_occ = 0
    pins, conflicts, book_differs, ineligible, in_book, ties, only_lost = {}, [], [], 0, 0, [], 0
    dropped_lost_moves = avoided_dropped = 0
    for fen, rs in rows.items():
        bmv = rs[0][4]
        if bmv != "-":
            in_book += 1
            diff = [r for r in rs if r[2] != bmv]
            if diff:
                book_differs.append((fen, bmv, diff))
            continue
        el = [r for r in rs if r[3]]
        if not el:
            ineligible += 1
            continue
        n_before = len(el)
        el = [r for r in el if not repaired(r)]  # losses repaired downstream: neither for nor against a move
        repaired_occ += n_before - len(el)
        if not el:
            only_lost += 1
            continue
        # a move is pinned only if no game through this position with this move was lost (lead 2026-11-11; before:
        # at least one non-lost occurrence); moves with a lost occurrence are dropped before the choice, and a position
        # left without a move is not pinned (nothing is pinned from a lost game)
        by_all = collections.defaultdict(list)
        for r in el:
            if r[2] in r[5]:  # never pin a move the base book avoids here
                avoided_dropped += 1
                continue
            by_all[r[2]].append(r)
        by = {m: v for m, v in by_all.items() if all(games[r[0]]["res"] != "L" for r in v)}
        if len(by) < len(by_all):
            dropped_lost_moves += len(by_all) - len(by)
        if not by:
            only_lost += 1
            continue
        # Wins first (lead 2026-11-11, r11d): among the moves without an unrepaired loss, a move from a game won against
        # Cake or KingsRow is preferred to moves that only drew, several such moves by their number of won games; a win
        # against GuiNN decides only where no candidate move of this position was ever played against Cake or KingsRow.
        # Then as before: worst result, frequency, run a, first met.
        ck_here = any(games[r[0]]["opp"] in ("cake", "kingsrow") for v in by.values() for r in v)
        def wins(mv):
            return sum(1 for r in by[mv] if games[r[0]]["res"] == "W"
                       and (games[r[0]]["opp"] in ("cake", "kingsrow") or (not ck_here and games[r[0]]["opp"] == "guinn")))
        def key(mv):
            gs = by[mv]
            worst = min(RANK[games[r[0]]["res"]] for r in gs)
            return (wins(mv), worst, len(gs), any(games[r[0]]["ra"] == "a" for r in gs))
        best = max(by, key=key)  # max() keeps the first move met among equal keys (frozen input order)
        if len(by) > 1 and sum(1 for m in by if key(m) == key(best)) > 1:
            ties.append(fen)
        if len(by) > 1:
            conflicts.append((fen, best, {m: [(games[r[0]]["run"], games[r[0]]["rnd"], games[r[0]]["res"]) for r in v]
                                          for m, v in by.items()}))
        g0 = games[by[best][0][0]]
        n = len(by[best])
        ress = collections.Counter(games[r[0]]["res"] for r in by[best])
        pins[fen] = (best, f"pin {g0['opp']} {g0['ballot']} regicide {g0['col']} ply {by[best][0][1]}: played in {n} "
                           f"final-validation game(s) (results {dict(ress)}), first {g0['run']} round {g0['rnd']}")
    with open(a.out, "w") as fo:
        fo.write("# WP11 pins: the moves of the final validation (bl-final1, runs a and b) in positions without a book "
                 "move, until 8 pieces.\n# FEN | move | force | evidence (opponent, ballot, colour, ply, games, results)\n")
        for fen, (mv, ev) in sorted(pins.items(), key=lambda kv: kv[1][1]):
            fo.write(f"{fen} | {mv} | force | {ev}\n")
    # statistics
    per_ballot = collections.Counter(ev.split(" regicide ")[0].split(" ", 2)[2] for _, ev in pins.values())
    md = [f"# WP11 pins from {len(games)} games", "",
          f"- positions with Regicide to move (ballot end to 8 pieces): {len(rows)} distinct",
          f"- already in book {os.path.basename(a.book)} (decision or old-book move): {in_book} (the played move differs in {len(book_differs)})",
          f"- never consulted (inside a run of king moves / database-decided): {ineligible}, left out",
          f"- occurrences from lost games repaired downstream by the base book (not counted): {repaired_occ}; occurrences of moves the base book avoids (never pinned): {avoided_dropped}",
          f"- positions where every move played there has a lost game through it: {only_lost}, left out; moves with a "
          f"lost game dropped in positions that keep another move: {dropped_lost_moves}",
          f"- **pins: {len(pins)}**; positions with different moves across games: {len(conflicts)}; of these, "
          f"{len(ties)} still tie after worst result, frequency and run a, and the first move met in the frozen input "
          f"order (runs sorted by name, games in file order) wins",
          f"- games without a final result skipped: {unfinished}; games rejected by the full replay: {len(rejected)}; "
          f"games skipped because not exactly one side is Regicide against a known opponent: {not_regicide}", ""]
    md.append("| Ballot | Pins |")
    md.append("|---|---|")
    for b, n in sorted(per_ballot.items(), key=lambda kv: -kv[1]):
        md.append(f"| {b} | {n} |")
    # first non-book move before / after, banked time
    def first_think(g):
        for i in range(g["nopen"], len(g["moves"])):
            if (i % 2 == 0) == (g["col"] == "B"):
                c = g["comments"][i]
                if c is not None and not is_book("regicide", c):
                    return i
        return None
    before, after, examples = [], [], []
    rejected_ids = {int(l.split("\t")[0]) for l in rejected}
    for gi, g in enumerate(games):
        if gi in rejected_ids:  # rejected by the full replay: not in the statistics either
            continue
        b = first_think(g)
        aft = None
        for ply, fen, mv, el, bmv, _av in per_game[gi]:
            # covered: the engine consults its book here (el) and r11 plays the game's move (existing entry or pin)
            covered = el and ((bmv != "-" and bmv == mv) or (fen in pins and pins[fen][0] == mv))
            if not covered:
                aft = ply
                break
        if aft is None:  # covered until 8 pieces or the end
            aft = per_game[gi][-1][0] + 2 if per_game[gi] else len(g["moves"])
        if b is not None:
            before.append(b)
            after.append(aft)
            if len(examples) < 6 and aft - b >= 20:
                # banked time (CheckerBoard): +0.5 s per own move, minus the time used; maxtime = 0.5 + (bank - 0.5)/2.5
                own_b = [i for i in range(g["nopen"], b) if (i % 2 == 0) == (g["col"] == "B")]
                used = 0.0
                for i in own_b:
                    m = TIME.match(g["comments"][i] or "")
                    used += float(m.group(1)) if m else 0.0
                bank_b = 0.5 * (len(own_b) + 1) - used
                own_a = [i for i in range(g["nopen"], aft) if (i % 2 == 0) == (g["col"] == "B")]
                bank_a = 0.5 * (len(own_a) + 1)
                examples.append(f"- {g['run']} round {g['rnd']} ({g['ballot']}, Regicide {g['col']}): first own "
                                f"non-book move of the old game at ply {b} with {bank_b:.1f} s banked (maxtime "
                                f"{0.5 + (bank_b - 0.5) / 2.5:.2f} s); first ply of the old line not covered by r11 at "
                                f"ply {aft}, {bank_a:.1f} s banked if every covered move is instant (maxtime "
                                f"{0.5 + (bank_a - 0.5) / 2.5:.2f} s)")
    if before:
        md += ["", f"**Coverage of the old line** (not r11's first own search, which only the r11 validation games show): "
               f"first own non-book move in the old game "
               f"mean ply {statistics.mean(before):.1f} (median {statistics.median(before):.0f}); first ply of the old "
               f"game not covered by r11 with the old game's move: mean {statistics.mean(after):.1f} (median "
               f"{statistics.median(after):.0f}), over {len(before)} games. Where the pin differs from the old game's "
               "move, r11 may still answer from the book after that point, so these figures are a coverage measure; the "
               "real first search and the bank at that point are measured in the r11 validation games.",
               "", "Bank estimates under the same caveat (CheckerBoard banking, 0.5 s per move; pinned moves instant):"] + examples
    else:
        md += ["", "**Coverage of the old line:** no usable game."]
    md += ["", "## Positions with different moves across games (chosen move first)", ""]
    for fen, best, d in conflicts:
        md.append(f"- `{fen}`: **{best}** " + "; ".join(f"{m}: {len(v)} game(s) {v[:3]}" for m, v in d.items()))
    md += ["", "## Positions where the existing book move differs from the played move (existing kept)", ""]
    for fen, bmv, diff in book_differs:
        md.append(f"- `{fen}`: book {bmv}; played " + ", ".join(f"{r[2]} ({games[r[0]]['run']} {games[r[0]]['rnd']}, "
                  f"{'eligible' if r[3] else 'book skipped by the engine'})" for r in diff[:4]))
    open(a.report, "w").write("\n".join(md) + "\n")
    print("\n".join(md[:12]))


if __name__ == "__main__":
    main()
