#!/usr/bin/env python3
"""Match order against the old order (lead 2026-11-11, 2b): which openings change result.

  order_compare.py --old RUNDIR ... --new RUNDIR ... [--md OUT]
Per opponent, opening and Regicide colour: our results in the OLD runs (e.g. r11d n8q, -c 8, games of a pair on
different workers) and in the NEW runs (match order, --pairs-together), and the openings whose result set differs
(a win or a loss in one order and not in the other). Also per opponent: W / D / L in each order and, for the new runs,
the result of the second game of each pair (Regicide White) against the first (Regicide Black).
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of, family  # noqa: E402


def load(dirs):
    out = collections.defaultdict(list)  # (opp, opening, colour) -> [(result, run round)]
    for d in dirs:
        for tags, _ in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            rb = tags.get("Black", "").strip().lower().startswith("regicide")
            rw = tags.get("White", "").strip().lower().startswith("regicide")
            if rb == rw:
                continue
            col = "B" if rb else "W"
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(
                tags.get("Result", "").strip(), "?")
            opp = family(tags["White"] if rb else tags["Black"])
            out[(opp, ballot_of(tags["Opening"])[0], col)].append((r, f"{os.path.basename(d)} {tags.get('Round')}"))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--old", nargs="+", required=True)
    ap.add_argument("--new", nargs="+", required=True)
    ap.add_argument("--md")
    a = ap.parse_args()
    old, new = load(a.old), load(a.new)
    out = ["| Opponent | old order W / D / L | match order W / D / L |", "|---|---|---|"]
    for opp in sorted({k[0] for k in new}):
        co = collections.Counter(r for k, v in old.items() if k[0] == opp for r, _ in v)
        cn = collections.Counter(r for k, v in new.items() if k[0] == opp for r, _ in v)
        out.append(f"| {opp} | {co['W']} / {co['D']} / {co['L']} ({sum(co.values())}) | {cn['W']} / {cn['D']} / {cn['L']} ({sum(cn.values())}) |")
    out += ["", "Openings whose results differ between the orders (a win or loss in one and not in the other):", "",
            "| Opponent | Opening | Regicide | old order | match order | games (decisive) |", "|---|---|---|---|---|---|"]
    rows = 0
    for k in sorted(set(old) | set(new)):
        so = collections.Counter(r for r, _ in old.get(k, []))
        sn = collections.Counter(r for r, _ in new.get(k, []))
        if {r for r in so if r != "D"} != {r for r in sn if r != "D"}:
            rows += 1
            dec = "; ".join(f"{r} {g}" for r, g in old.get(k, []) + new.get(k, []) if r != "D")
            out.append(f"| {k[0]} | {k[1]} | {k[2]} | {so['W']} / {so['D']} / {so['L']} | {sn['W']} / {sn['D']} / {sn['L']} | {dec} |")
    if not rows:
        out.append("| — | | | | | |")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
