#!/usr/bin/env python3
"""Time per move from the PDN comments (lead 2026-11-11: the clock under load, concurrency-1 runs against 8-worker runs).

  clock_report.py RUNDIR ... [--md OUT]
Per run and engine (Regicide / opponent): moves with a time in their comment, of them book or instant moves (< 0.02 s),
mean time over all timed moves and over searched moves, the 95th percentile and maximum, and the time used against the
time credited by the 0.5 s/move setting (CheckerBoard banking: every move credits 0.5 s; an engine that uses less banks
the rest, so the mean over all moves must stay below 0.5 s and the ratio below 1).
"""
import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn  # noqa: E402

T = re.compile(r"^\s*([\d.]+)s\b")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--md")
    a = ap.parse_args()
    out = ["| Run | engine | timed moves | instant (< 0.02 s) | mean, all | mean, searched | p95 | max | used / credited |",
           "|---|---|---|---|---|---|---|---|---|"]
    for d in a.runs:
        side = {"Regicide": [], "opponent": []}
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            rb = tags.get("Black", "").lower().startswith("regicide")
            for i, (_, c) in enumerate(moves):
                m = T.match(c or "")
                if m:
                    side["Regicide" if (i % 2 == 0) == rb else "opponent"].append(float(m.group(1)))
        for k, ts in side.items():
            if not ts:
                continue
            s = sorted(ts)
            srch = [t for t in ts if t >= 0.02]
            out.append(f"| {os.path.basename(d.rstrip('/'))} | {k} | {len(ts)} | {len(ts) - len(srch)} | {sum(ts) / len(ts):.3f} s | "
                       f"{(sum(srch) / len(srch) if srch else 0):.3f} s | {s[int(0.95 * (len(s) - 1))]:.2f} s | {s[-1]:.2f} s | "
                       f"{sum(ts) / (0.5 * len(ts)):.2f} |")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
