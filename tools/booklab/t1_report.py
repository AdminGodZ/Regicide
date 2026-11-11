#!/usr/bin/env python3
"""T1 report (experiments/time-policy/README.md on master ac4bd20): tm_soft 100 against 60, Cake, H3.

  t1_report.py --results DIR --repeats a [b ...] [--boot 10000] [--md OUT]
Runs bl-t1<rep>-<arm>-cake-H3_<k> for arm 60 and 100, played with bin/match-pr49-movelog (the PR #49 runner plus a
per-move log, tools/booklab/patches/match_movelog.diff): games/<n>.pdn.moves.tsv holds, for every engine move, the
runner's own bank before the move (this move's credit included), the offered maxtime, the runner's wall time, the
host's time and the time the runner charged. Nothing is reconstructed (review 2026-11-11).

Read-out (README): W/D/L per arm; the paired score difference 100 minus 60 in match-score points over an arm's 312
games (3 points = 0.96 percentage points), with a 95% interval that resamples the 156 openings (both colours of an
opening stay together); with a second run, each opening's difference is the mean of the runs. Time used by Regicide
per phase on searched moves (host time; > 0.02 s), its share of the offered maxtime, and the bank before Regicide's
first move with 8 or fewer pieces on the board. The decision rule is the README's; this script does not decide.
"""
import argparse
import collections
import csv
import glob
import os
import random
import re
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

PHASES = [(21, 24, "21-24"), (17, 20, "17-20"), (13, 16, "13-16"), (9, 12, "9-12"), (0, 8, "<=8")]


def pieces_after(moves):
    out, n = [24], 24
    for m in moves:
        if "x" in m:
            n -= len(m.split("x")) - 1
        out.append(n)
    return out


def load_run(d):
    """[(opening, regicide colour, score, moves, movelog rows)] for every game of a run directory."""
    games = []
    for p in sorted(glob.glob(os.path.join(d, "games", "*.pdn"))):
        g = list(parse_pdn(open(p, errors="replace").read()))
        if len(g) != 1:
            sys.exit(f"{p}: {len(g)} games")
        tags, mv = g[0]
        is_reg = [n.strip().lower().startswith("regicide") for n in (tags.get("Black", ""), tags.get("White", ""))]
        if sum(is_reg) != 1:
            sys.exit(f"{p}: not exactly one player named Regicide ({tags.get('Black')} / {tags.get('White')})")
        col = "B" if is_reg[0] else "W"
        r = tags.get("Result", "").strip()
        sc = {"1-0": 1.0 if col == "B" else 0.0, "0-1": 1.0 if col == "W" else 0.0, "1/2-1/2": 0.5}.get(r)
        log = p + ".moves.tsv"
        if not os.path.exists(log):
            sys.exit(f"{log} missing: the run was not played with match-pr49-movelog")
        rows = list(csv.DictReader(open(log), delimiter="\t"))
        # first own move whose search score is below -100 (Regicide's comment 'score N')
        drop = next((i + 1 for i, (m, c) in enumerate(mv) if (i % 2 == 0) == (col == "B")
                     and (s := re.search(r"\bscore (-?\d+)", c or "")) and int(s.group(1)) < -100), None)
        info = dict(ballot=ballot_of(tags["Opening"])[0], term=tags.get("Termination", ""), drop=drop,
                    game=f"{os.path.basename(d)} round {tags.get('Round', '?')}")
        games.append((ballot_of(tags["Opening"])[1], col, sc, [m for m, _ in mv], rows, info))
    return games


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--repeats", nargs="+", default=["a"])
    ap.add_argument("--boot", type=int, default=10000)
    ap.add_argument("--opp", default="cake", help="cake (holdout H3) or kingsrow (production deck)")
    ap.add_argument("--parts", nargs="+", default=None, help="run parts (default H3_0 H3_1 for cake, h1 h2 otherwise)")
    ap.add_argument("--md")
    a = ap.parse_args()
    parts = a.parts or (["H3_0", "H3_1"] if a.opp == "cake" else ["h1", "h2"])
    score = {}  # (rep, arm) -> {(opening, colour): score}
    tim, share, bank8, wdl = (collections.defaultdict(lambda: collections.defaultdict(list)),
                              collections.defaultdict(lambda: collections.defaultdict(list)),
                              collections.defaultdict(list), collections.defaultdict(collections.Counter))
    lost, forfeits = collections.defaultdict(list), collections.Counter()
    # Completeness first (review 2026-11-11): every arm, repeat and part must have exit status 0, a complete summary
    # ("Games: N of N scheduled" and the A: line) and N game files; and both arms of a repeat must have the same full
    # set of opening and colour keys. Otherwise nothing is computed.
    problems = []
    keysets = collections.defaultdict(set)
    for rep in a.repeats:
        for arm in ("60", "100"):
            for part in parts:
                d = os.path.join(a.results, f"bl-t1{rep}-{arm}-{a.opp}-{part}")
                es = open(f"{d}/exit_status").read().strip() if os.path.exists(f"{d}/exit_status") else None
                sm = open(f"{d}/summary.txt").read() if os.path.exists(f"{d}/summary.txt") else ""
                m = re.search(r"^Games: (\d+) of (\d+) scheduled", sm, re.M)
                n = len(glob.glob(os.path.join(d, "games", "*.pdn")))
                if es != "0":
                    problems.append(f"{d}: exit status {es}")
                if not m or m.group(1) != m.group(2) or not re.search(r"^A:", sm, re.M):
                    problems.append(f"{d}: summary incomplete ({m.group(0) if m else 'no Games line'})")
                elif int(m.group(1)) != n:
                    problems.append(f"{d}: {n} game files for {m.group(1)} games")
                for g in glob.glob(os.path.join(d, "games", "*.pdn")):
                    for tags, _ in parse_pdn(open(g, errors="replace").read()):
                        reg_black = tags.get("Black", "").strip().lower().startswith("regicide")
                        keysets[(rep, arm)].add((ballot_of(tags["Opening"])[1], "B" if reg_black else "W"))
        if keysets[(rep, "60")] != keysets[(rep, "100")]:
            problems.append(f"repeat {rep}: the two arms have different opening/colour sets "
                            f"({len(keysets[(rep, '60')])} / {len(keysets[(rep, '100')])}, "
                            f"{len(keysets[(rep, '60')] ^ keysets[(rep, '100')])} differ)")
    if problems:
        sys.exit("T1 report refused, incomplete input:\n  " + "\n  ".join(problems))
    for rep in a.repeats:
        for arm in ("60", "100"):
            score[(rep, arm)] = {}
            for part in parts:
                for opening, col, sc, moves, rows, info in load_run(os.path.join(a.results, f"bl-t1{rep}-{arm}-{a.opp}-{part}")):
                    if sc == 0.0:
                        lost[arm].append(f"{info['ballot']} (Regicide {col}): score below −100 from ply "
                                         f"{info['drop'] or '— (never)'}; {info['term']}; {info['game']}")
                    # a time forfeit is a game Regicide lost by the runner's time termination (review 2026-11-11:
                    # not a game the opponent lost on time)
                    if sc == 0.0 and any(k in info["term"].lower() for k in ("lost on time", "timed out", "time forfeit")):
                        forfeits[arm] += 1
                    key = (opening, col)
                    if key in score[(rep, arm)]:
                        sys.exit(f"duplicate {key} in repeat {rep}, arm {arm}")
                    score[(rep, arm)][key] = sc
                    wdl[arm][{1.0: "W", 0.5: "D", 0.0: "L", None: "?"}[sc]] += 1
                    pcs = pieces_after(moves)
                    seen8 = False
                    for row in rows:
                        if row["side"] != col:
                            continue
                        ply = int(row["ply"])
                        if pcs[ply] <= 8 and not seen8:
                            bank8[arm].append(float(row["bank_before"]))
                            seen8 = True
                        ht = float(row["host_time"])
                        if ht < 0.02:
                            continue
                        ph = next(name for lo, hi, name in PHASES if lo <= pcs[ply] <= hi)
                        tim[arm][ph].append(ht)
                        share[arm][ph].append(ht / float(row["maxtime"]))
    # paired difference per opening, mean over repeats
    per_open = collections.defaultdict(list)  # opening -> [difference of one repeat (both colours summed)]
    missing = 0
    for rep in a.repeats:
        d = collections.defaultdict(float)
        for key, s60 in score[(rep, "60")].items():
            s100 = score[(rep, "100")].get(key)
            if s60 is None or s100 is None:
                missing += 1
                continue
            d[key[0]] += s100 - s60
        for op, v in d.items():
            per_open[op].append(v)
    diffs = [statistics.mean(v) for v in per_open.values()]
    total = sum(diffs)
    rnd = random.Random(2026111101)
    boots = sorted(sum(rnd.choice(diffs) for _ in diffs) for _ in range(a.boot))
    lo, hi = boots[int(0.025 * a.boot)], boots[int(0.975 * a.boot) - 1]
    games = sum(len(score[(a.repeats[0], "60")]) for _ in [0])
    out = [f"# T1 report: tm_soft 100 minus 60, {'Cake, holdout H3' if a.opp == 'cake' else a.opp + ', production deck'}, "
           f"repeats {', '.join(a.repeats)}", ""]
    for arm in ("60", "100"):
        out.append(f"- tm_soft {arm}: time forfeits {forfeits[arm]}; losses {len(lost[arm])}" +
                   "".join(f"\n  - {x}" for x in lost[arm]))
    out.append("")
    for arm in ("60", "100"):
        c = wdl[arm]
        out.append(f"- tm_soft {arm}: Regicide W / D / L = {c['W']} / {c['D']} / {c['L']}" + (f", {c['?']} without result" if c["?"] else ""))
    out += ["", f"**Paired score difference (100 − 60): {total:+.1f} match-score points over {games} games per arm** "
            f"({100 * total / games:+.2f} percentage points); 95% interval resampling the {len(diffs)} openings "
            f"[{lo:+.1f}, {hi:+.1f}]" + (f"; mean of {len(a.repeats)} runs within each opening" if len(a.repeats) > 1 else "")
            + (f"; {missing} pairs without a result left out" if missing else "") + ".", "",
            "Regicide's time on searched moves (host time > 0.02 s) by pieces on the board: mean host time, median share "
            "of the offered maxtime [moves]:", "", "| Pieces | tm_soft 60 | tm_soft 100 |", "|---|---|---|"]
    for _, _, ph in PHASES:
        cells = []
        for arm in ("60", "100"):
            x = tim[arm][ph]
            cells.append(f"{statistics.mean(x):.2f} s, {100 * statistics.median(share[arm][ph]):.0f}% [{len(x)}]" if x else "-")
        out.append(f"| {ph} | {cells[0]} | {cells[1]} |")
    out += ["", "Runner's bank before Regicide's first move with 8 or fewer pieces (this move's credit included), "
            "median / mean [games]:", ""]
    for arm in ("60", "100"):
        b = bank8[arm]
        out.append(f"- tm_soft {arm}: {statistics.median(b):.2f} s / {statistics.mean(b):.2f} s [{len(b)}]" if b else f"- tm_soft {arm}: -")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
