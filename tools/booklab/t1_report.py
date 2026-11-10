#!/usr/bin/env python3
"""T1 report (experiments/time-policy/README.md): tm_soft 60 against 100, Cake, H3, paired by opening and colour.

  t1_report.py --results DIR --shim DIR --repeat a [--rate R] [--movetime 0.5] [--md OUT]
Runs bl-t1<rep>-<arm>-cake-H3_<k> (arm 60 and 100); shim logs <shim>/t1<rep>-<arm>-H3_<k>/{A,B}/*.tsv.

Per move (from the shim logs of Regicide, side A): wall time (go to bestmove at the shim, WSL clock, times R to
convert to Windows seconds), host time S ("bestmove ... time S", Windows clock), the charge (PR #49 runner:
min(S, runner wall); the runner's wall is the shim interval plus two pipe hops, taken as the shim interval), the
offered maxtime (from the `go maxtime X` line) and the bank before the move (CheckerBoard banking:
maxtime = M + (bank - M) / 2.5, so bank = M + 2.5 (maxtime - M)). Moves are joined to the games by the move list
of the `position` line; pieces on the board from the replayed game.
Read-out: W/D/L per arm; paired score difference (100 minus 60) by opening (both colours summed) with a 95%
interval (normal, over openings); time used per phase on searched moves (> 0.02 s) by pieces on the board;
bank before the first own move at <= 8 pieces. The decision rule is the README's; this script does not decide.
"""
import argparse
import collections
import glob
import math
import os
import re
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

PHASES = [(21, 24, "21-24"), (17, 20, "17-20"), (13, 16, "13-16"), (9, 12, "9-12"), (0, 8, "<=8")]


def pieces_after(moves):
    """Pieces on the board after each prefix: 24 minus captured men (a capture's jumps: path length - 1)."""
    out, n = [24], 24
    for m in moves:
        if "x" in m:
            n -= len(m.split("x")) - 1
        out.append(n)
    return out


def shim_moves(d):
    """[(position move list, wall, S, maxtime)] for every searched move of the process logs in d."""
    res = []
    for f in sorted(glob.glob(os.path.join(d, "*.tsv"))):
        pos = go_t = mt = None
        for l in open(f, errors="replace"):
            t, direc, line = l.rstrip("\n").split("\t", 2)
            t = float(t)
            if direc == ">" and line.startswith("position"):
                pos = line.split(" moves ", 1)[1] if " moves " in line else ""
            elif direc == ">" and line.startswith("go"):
                go_t = t
                m = re.search(r"maxtime ([0-9.]+)", line)
                mt = float(m.group(1)) if m else None
            elif direc == "<" and line.startswith("bestmove") and go_t is not None:
                m = re.search(r"\btime ([0-9.]+)", line)
                res.append((pos, t - go_t, float(m.group(1)) if m else None, mt))
                go_t = None
    return res


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--shim", required=True)
    ap.add_argument("--repeat", default="a")
    ap.add_argument("--rate", type=float, default=1.0)
    ap.add_argument("--movetime", type=float, default=0.5)
    ap.add_argument("--md")
    a = ap.parse_args()
    M = a.movetime
    res = {}  # arm -> {(opening, colour): result}
    tim = {}  # arm -> phase -> [S]
    tim_wall, share = {}, {}
    bank8 = {}
    for arm in ("60", "100"):
        res[arm], tim[arm], tim_wall[arm], share[arm], bank8[arm] = {}, collections.defaultdict(list), \
            collections.defaultdict(list), collections.defaultdict(list), []
        for part in ("H3_0", "H3_1"):
            run = f"bl-t1{a.repeat}-{arm}-cake-{part}"
            moves_by_pos = {}
            for pos, wall, s, mt in shim_moves(os.path.join(a.shim, f"t1{a.repeat}-{arm}-{part}", "A")):
                moves_by_pos[pos] = (wall * a.rate, s, mt)
            for tags, mv in parse_pdn(open(os.path.join(a.results, run, "games.pdn"), errors="replace").read()):
                col = "B" if family(tags["Black"]) == "regicide" else "W"
                r = tags.get("Result", "").strip()
                sc = {"1-0": 1.0 if col == "B" else 0.0, "0-1": 1.0 if col == "W" else 0.0, "1/2-1/2": 0.5}.get(r)
                key = (ballot_of(tags["Opening"])[1], col)
                if key in res[arm]:
                    sys.exit(f"duplicate {key} in arm {arm}")
                res[arm][key] = sc
                moves = [m for m, _ in mv]
                pcs = pieces_after(moves)
                seen8 = False
                for i in range(len(moves)):
                    if (i % 2 == 0) != (col == "B"):
                        continue
                    rec = moves_by_pos.get(" ".join(moves[:i]))
                    if rec is None:
                        continue
                    wall, s, mt = rec
                    if pcs[i] <= 8 and not seen8 and mt is not None:
                        bank8[arm].append(M + 2.5 * (mt - M))
                        seen8 = True
                    if s is None or s < 0.02:
                        continue
                    ph = next(name for lo, hi, name in PHASES if lo <= pcs[i] <= hi)
                    tim[arm][ph].append(s)
                    tim_wall[arm][ph].append(wall)
                    if mt:
                        share[arm][ph].append(s / mt)
    out = [f"# T1 report, repeat {a.repeat} (tm_soft 100 minus 60), Cake, H3", ""]
    for arm in ("60", "100"):
        c = collections.Counter(res[arm].values())
        out.append(f"- tm_soft {arm}: {len(res[arm])} games, Regicide W / D / L = {c[1.0]} / {c[0.5]} / {c[0.0]}"
                   + (f" ({sum(1 for v in res[arm].values() if v is None)} without result)" if None in res[arm].values() else ""))
    by_open = collections.defaultdict(float)
    pairs = better = worse = 0
    for k in res["60"]:
        if k in res["100"] and res["60"][k] is not None and res["100"][k] is not None:
            d = res["100"][k] - res["60"][k]
            by_open[k[0]] += d
            pairs += 1
            better += d > 0
            worse += d < 0
    diffs = list(by_open.values())
    tot = sum(diffs)
    se = statistics.pstdev(diffs) * math.sqrt(len(diffs)) if len(diffs) > 1 else float("nan")
    out += ["", f"Paired by opening and colour: {pairs} pairs, 100 better in {better}, worse in {worse}; score difference "
            f"100 − 60 = **{tot:+.1f} points**, 95% interval by opening [{tot - 1.96 * se:+.1f}, {tot + 1.96 * se:+.1f}] "
            f"({len(diffs)} openings).", "",
            "Time on searched moves (> 0.02 s) by pieces on the board: host time mean (wall time mean), share of the "
            "offered maxtime (median):", "", "| Pieces | tm_soft 60 | tm_soft 100 |", "|---|---|---|"]
    for _, _, ph in PHASES:
        cells = []
        for arm in ("60", "100"):
            x = tim[arm][ph]
            cells.append(f"{statistics.mean(x):.2f} s ({statistics.mean(tim_wall[arm][ph]):.2f} s), "
                         f"{100 * statistics.median(share[arm][ph]):.0f}% [{len(x)} moves]" if x else "-")
        out.append(f"| {ph} | {cells[0]} | {cells[1]} |")
    out += ["", "Bank before Regicide's first move with 8 or fewer pieces on the board (median / mean, games):", ""]
    for arm in ("60", "100"):
        b = bank8[arm]
        out.append(f"- tm_soft {arm}: {statistics.median(b):.2f} s / {statistics.mean(b):.2f} s ({len(b)} games)" if b else
                   f"- tm_soft {arm}: -")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
