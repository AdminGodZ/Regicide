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
    games, lines, unfinished = [], [], 0
    # Input order is frozen: runs sorted by name, games in file order. It decides the remaining ties (after worst
    # result, frequency and run a, the first move met wins).
    for d in sorted(a.runs):
        run = os.path.basename(d.rstrip("/"))  # bl-f1-cake-a-h1
        p = run.split("-")
        opp, ra = p[2], p[3]
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            col = "B" if family(tags["Black"]) == "regicide" else "W"
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
        rows[fen].append((gi, ply, mv, el, bmv))
        per_game[gi].append((ply, fen, mv, el, bmv))
    pins, conflicts, book_differs, ineligible, in_book, ties, only_lost = {}, [], [], 0, 0, [], 0
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
        # a pin needs at least one won or drawn occurrence: moves met only in lost games are not pinned
        if all(games[r[0]]["res"] == "L" for r in el):
            only_lost += 1
            continue
        by = collections.defaultdict(list)
        for r in el:
            by[r[2]].append(r)
        def key(mv):
            gs = by[mv]
            worst = min(RANK[games[r[0]]["res"]] for r in gs)
            return (worst, len(gs), any(games[r[0]]["ra"] == "a" for r in gs))
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
          f"- already in book r10 (decision or old-book move): {in_book} (the played move differs in {len(book_differs)})",
          f"- never consulted (inside a run of king moves / database-decided): {ineligible}, left out",
          f"- met only in lost games: {only_lost}, left out (a pin needs a won or drawn occurrence)",
          f"- **pins: {len(pins)}**; positions with different moves across games: {len(conflicts)}; of these, "
          f"{len(ties)} still tie after worst result, frequency and run a, and the first move met in the frozen input "
          f"order (runs sorted by name, games in file order) wins",
          f"- games without a final result skipped: {unfinished}; games rejected by the full replay: {len(rejected)}", ""]
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
        for ply, fen, mv, el, bmv in per_game[gi]:
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
    md += ["", f"**Coverage of the old line** (not r11's first own search, which only the r11 validation games show): "
           f"first own non-book move in the old game "
           f"mean ply {statistics.mean(before):.1f} (median {statistics.median(before):.0f}); first ply of the old game "
           f"not covered by r11 with the old game's move: mean {statistics.mean(after):.1f} (median "
           f"{statistics.median(after):.0f}), over {len(before)} games. Where the pin differs from the old game's move, r11 "
           "may still answer from the book after that point, so these figures are a coverage measure; the real first "
           "search and the bank at that point are measured in the r11 validation games.",
           "", "Bank estimates under the same caveat (CheckerBoard banking, 0.5 s per move; pinned moves instant):"] + examples
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
