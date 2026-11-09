#!/usr/bin/env python3
"""WP1 census report from match-runner result dirs.

  census.py --results ~/regicide-match/results --out DIR PAIRING=TAG1,TAG2 ...  [--repeat NAME=TAG:REPTAG ...]

For each pairing (several result dirs pooled, e.g. the two halves of acf156): W/D/L from A's side, Elo with a 95% CI
from the pentanomial pair scores (as tools/match/stats.cpp), every decisive game (ballot, colours, winner), and per
engine and ballot the number of its leading moves (after the ballot) that took under 0.05 s (instant: book or
forced). --repeat compares two runs of the same pairing game by game (same ballot and colours): identical games, and
the first ply where they differ.
Writes DIR/census.md (tables) and DIR/instant_by_ballot.csv.
"""
import argparse
import collections
import csv
import math
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of, TIME, is_book  # noqa: E402

NAMES = {"cake": "Cake", "kingsrow": "KingsRow", "guinn": "GuiNN", "regicide": "Regicide"}


def games_of(results, tags):
    out = []
    for t in tags:
        f = os.path.join(results, t, "games.pdn")
        if not os.path.exists(f):
            print(f"missing {f}", file=sys.stderr)
            continue
        for tags_, moves in parse_pdn(open(f, errors="replace").read()):
            out.append((t, tags_, moves))
    return out


def elo(s):
    s = min(max(s, 1e-6), 1 - 1e-6)
    return -400 * math.log10(1 / s - 1)


def pent_elo(pairs):
    sc = [p / 2 for p in pairs]  # pair score in [0, 1]
    n = len(sc)
    m = sum(sc) / n
    var = sum((x - m) ** 2 for x in sc) / max(1, n - 1)
    se = math.sqrt(var / n)
    return elo(m), elo(m - 1.96 * se), elo(m + 1.96 * se)


def leading_instant(moves, nopen, side_first, fams=None):
    """Count, per side ('B'/'W'), the leading moves after the ballot with t < 0.05 s (or, with fams, the leading
    moves the engine's comment marks as book moves)."""
    res = {}
    for col in "BW":
        n = 0
        for ply in range(nopen, len(moves)):
            side = side_first if ply % 2 == 0 else ("W" if side_first == "B" else "B")
            if side != col:
                continue
            c = moves[ply][1]
            m = TIME.match(c) if c else None
            if fams is not None:
                ok = is_book(fams[col], c)
            else:
                ok = m and float(m.group(1)) < 0.05
            if ok:
                n += 1
            else:
                break
        res[col] = n
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", default=os.path.expanduser("~/regicide-match/results"))
    ap.add_argument("--out", required=True)
    ap.add_argument("--repeat", action="append", default=[])
    ap.add_argument("pairings", nargs="+")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    md = []
    inst = collections.defaultdict(list)  # (engine, ballot, colour) -> [n]
    book = collections.defaultdict(list)  # same, leading book-marked moves
    ballots = {}
    md.append("| Pairing (A vs B) | Games | A: W / D / L | Elo of A [95% CI] | Decisive games (ballot, winner as colour) |")
    md.append("|---|---|---|---|---|")
    decisive_all = []
    for p in a.pairings:
        name, tags = p.split("=", 1)
        gs = games_of(a.results, tags.split(","))
        if not gs:
            continue
        an = None
        w = d = l = 0
        pairs = collections.defaultdict(float)
        dec = []
        for t, tg, moves in gs:
            b, wh = tg.get("Black", ""), tg.get("White", "")
            if an is None:
                ev = tg.get("Event", "")
                an = ev.split(" vs ")[0] if " vs " in ev else b
            a_col = "B" if b == an else "W"
            res = tg.get("Result", "")
            bname, bmoves, nopen = ballot_of(tg.get("Opening", ""))
            ballots[bmoves] = bname
            sc = 0.5
            if res == "1-0":
                sc = 1.0 if a_col == "B" else 0.0
            elif res == "0-1":
                sc = 1.0 if a_col == "W" else 0.0
            w += sc == 1.0
            d += sc == 0.5
            l += sc == 0.0
            pairs[(t, tg.get("Round", "").split(".")[0])] += sc
            if res in ("1-0", "0-1"):
                win_col = "B" if res == "1-0" else "W"
                winner = b if win_col == "B" else wh
                dec.append(f"{bname} ({winner} as {'Black' if win_col == 'B' else 'White'})")
                decisive_all.append((name, bname, winner, win_col, t, tg.get("Round", "")))
            li = leading_instant(moves, nopen, "B")
            lb = leading_instant(moves, nopen, "B", {"B": family(b), "W": family(wh)})
            for col, nm in (("B", b), ("W", wh)):
                inst[(family(nm), bmoves, col)].append(li[col])
                book[(family(nm), bmoves, col)].append(lb[col])
        e, lo, hi = pent_elo(list(pairs.values()))
        md.append(f"| {name} | {w + d + l} | {w} / {d} / {l} | {e:+.1f} [{lo:+.1f}, {hi:+.1f}] | "
                  f"{'; '.join(dec) if dec else '-'} |")
    md.append("")
    # Instant leading moves (book depth) per engine.
    md.append("**Leading instant moves after the ballot (< 0.05 s), per engine and colour, over all census games:**")
    md.append("")
    md.append("| Engine | As | Games | Mean | Median | 0 | 1-4 | 5-9 | 10-19 | 20+ | Leading book-marked moves: mean / median |")
    md.append("|---|---|---|---|---|---|---|---|---|---|---|")
    per_eng = collections.defaultdict(list)
    per_book = collections.defaultdict(list)
    for (eng, bm, col), v in inst.items():
        per_eng[(eng, col)] += v
        per_book[(eng, col)] += book[(eng, bm, col)]
    for (eng, col), v in sorted(per_eng.items()):
        bins = [sum(1 for x in v if x == 0), sum(1 for x in v if 1 <= x <= 4), sum(1 for x in v if 5 <= x <= 9),
                sum(1 for x in v if 10 <= x <= 19), sum(1 for x in v if x >= 20)]
        bv = per_book[(eng, col)]
        md.append(f"| {NAMES.get(eng, eng)} | {'Black' if col == 'B' else 'White'} | {len(v)} | "
                  f"{statistics.mean(v):.1f} | {statistics.median(v):.0f} | " + " | ".join(map(str, bins))
                  + f" | {statistics.mean(bv):.1f} / {statistics.median(bv):.0f} |")
    md.append("")
    engines = sorted({k[0] for k in inst})
    with open(os.path.join(a.out, "instant_by_ballot.csv"), "w", newline="") as f:
        cw = csv.writer(f)
        cw.writerow(["ballot", "moves"] + [f"{e}_{c}_{k}" for e in engines for c in "BW" for k in ("instant", "book")])
        for bm, bn in ballots.items():
            row = [bn, bm]
            for e in engines:
                for c in "BW":
                    for dd in (inst, book):
                        v = dd.get((e, bm, c))
                        row.append(f"{statistics.mean(v):.1f}" if v else "")
            cw.writerow(row)
    # Determinism.
    for r in a.repeat:
        name, spec = r.split("=", 1)
        t1, t2 = spec.split(":")
        g1 = {(tg.get("Opening", ""), tg.get("Black", "")): mv for _, tg, mv in games_of(a.results, [t1])}
        g2 = {(tg.get("Opening", ""), tg.get("Black", "")): mv for _, tg, mv in games_of(a.results, [t2])}
        same, diffs = 0, []
        for k, m1 in g1.items():
            m2 = g2.get(k)
            if m2 is None:
                continue
            s1, s2 = [x[0] for x in m1], [x[0] for x in m2]
            if s1 == s2:
                same += 1
                continue
            i = next((i for i in range(min(len(s1), len(s2))) if s1[i] != s2[i]), min(len(s1), len(s2)))
            diffs.append(i - ballot_of(k[0])[2])
        n = same + len(diffs)
        md.append(f"**Determinism, {name}** ({t1} vs {t2}, {n} games with the same ballot and colours): "
                  f"{same} identical ({100 * same / max(1, n):.0f}%); the others first differ at a median of "
                  f"{statistics.median(diffs) if diffs else 0:.0f} plies after the ballot "
                  f"(quartiles {statistics.quantiles(diffs, n=4)[0] if len(diffs) > 1 else '-'}–"
                  f"{statistics.quantiles(diffs, n=4)[2] if len(diffs) > 1 else '-'}; "
                  f"min {min(diffs) if diffs else '-'}).")
        md.append("")
    with open(os.path.join(a.out, "decisive.csv"), "w", newline="") as f:
        cw = csv.writer(f)
        cw.writerow(["pairing", "ballot", "winner", "winner_colour", "run", "round"])
        cw.writerows(decisive_all)
    open(os.path.join(a.out, "census.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
