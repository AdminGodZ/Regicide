#!/usr/bin/env python3
"""Report for acct_run.sh: per engine, the difference between the time the runner charged and the time the host
reports for getmove ("bestmove ... time S").

  cbshim_report.py ACCTDIR RESULTDIR...
ACCTDIR/<opp>/{A,B}/<pid>.tsv: shim logs (A = Regicide, B = the opponent). For every "go" line and the next
"bestmove" line of the same process: shim interval = t(bestmove) - t(go) (what the runner measures, minus the last
pipe hop to the runner); host S. Also, from the games' PDN comments (the runner's own charge, 2 decimals), matched
to the host times move by move in each game: runner charge - S.
"""
import os
import re
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family  # noqa: E402

TIME = re.compile(r"^(\d+\.\d+)s")


def shim_rows(d):
    rows = []  # (go wall interval, position+go interval, host S)
    for f in sorted(os.listdir(d)):
        t_pos = t_go = None
        for l in open(os.path.join(d, f), errors="replace"):
            t, direc, line = l.rstrip("\n").split("\t", 2)
            t = float(t)
            if direc == ">" and line.startswith("position"):
                t_pos = t
            elif direc == ">" and line.startswith("go"):
                t_go = t
            elif direc == "<" and line.startswith("bestmove") and t_go is not None:
                m = re.search(r"\btime ([0-9.]+)", line)
                if m:
                    rows.append((t - t_go, t - (t_pos if t_pos is not None else t_go), float(m.group(1))))
                t_go = t_pos = None
    return rows


def stats(xs):
    return f"{statistics.mean(xs) * 1000:.1f} ms / {max(xs) * 1000:.1f} ms" if xs else "-"


def main():
    # optional --rate R: Windows seconds per WSL monotonic second (the shim and the runner time with the WSL clock;
    # on Win11-AdminGod 2026-11-10 the WSL clocksource tsc ran 7.9 % slow against Windows QPC: R = 1.0788)
    args = sys.argv[1:]
    rate = 1.0
    if args and args[0] == "--rate":
        rate, args = float(args[1]), args[2:]
    acct, runs = args[0], args[1:]
    names = {"cake": "Cake", "kingsrow": "KingsRow", "guinn": "GuiNN"}
    out = ["# Runner wall time minus host-reported getmove time, per move (old accounting)", "",
           f"Shim intervals multiplied by {rate} (Windows seconds per WSL second) before the comparison." if rate != 1.0
           else "Shim intervals in WSL seconds, uncorrected.", "",
           "Shim interval: from the runner's `go` line reaching the shim to the host's `bestmove` line leaving it "
           "(the runner's own clock adds one pipe hop on each side). Position interval: from the `position ...` "
           "line (with the game history) instead. Runner charge: the time in the PDN comment (2 decimals).", "",
           "| Engine (match) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max | host S mean |",
           "|---|---|---|---|---|"]
    per_engine = {}
    for opp in ("cake", "kingsrow", "guinn"):
        for side, eng in (("A", "Regicide"), ("B", names[opp])):
            d = os.path.join(acct, opp, side)
            if not os.path.isdir(d):
                continue
            rows = [(w * rate, pw * rate, hs) for w, pw, hs in shim_rows(d)]
            per_engine.setdefault(eng, []).extend(rows)
            out.append(f"| {eng} (vs {names[opp]}) | {len(rows)} | {stats([r[0] - r[2] for r in rows])} | "
                       f"{stats([r[1] - r[2] for r in rows])} | {statistics.mean(r[2] for r in rows) * 1000:.0f} ms |")
    out += ["", "| Engine (all matches) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max |",
            "|---|---|---|---|"]
    for eng, rows in per_engine.items():
        out.append(f"| {eng} | {len(rows)} | {stats([r[0] - r[2] for r in rows])} | {stats([r[1] - r[2] for r in rows])} |")
    # runner charge from the PDN comments, per engine
    charge = {}
    for d in runs:
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            for i, (mv, c) in enumerate(moves):
                m = TIME.match(c or "")
                if m:
                    who = tags["Black"] if i % 2 == 0 else tags["White"]
                    charge.setdefault(family(who) or "regicide", []).append(float(m.group(1)))
    out += ["", "Runner charge per move from the PDN comments (all moves incl. book moves): "
            + "; ".join(f"{k} mean {statistics.mean(v) * 1000:.0f} ms, max {max(v):.2f} s ({len(v)} moves)"
                        for k, v in sorted(charge.items()))]
    print("\n".join(out))


if __name__ == "__main__":
    main()
