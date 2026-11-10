#!/usr/bin/env python3
"""KingsRow in book mode 3 (review A2): W/D/L of the mode-3 runs, determinism between them, and a game-by-game
comparison with the mode-1 final-validation runs.

  k3_compare.py --results DIR --mode3 k3-kingsrow-a k3-kingsrow-b --mode1 f1-kingsrow-a f1-kingsrow-b [--md OUT]
Each name X stands for the two halves bl-X-h1 and bl-X-h2. Games are matched by (opening, Regicide's colour).
For every pair of runs: identical games, and for the others the first differing ply (one-based, ballot included)
and whose move it was (Regicide or KingsRow). Results are from Regicide's view.
"""
import argparse
import collections
import os
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402


def load(results, name):
    g = {}
    for h in ("h1", "h2"):
        for tags, moves in parse_pdn(open(os.path.join(results, f"bl-{name}-{h}", "games.pdn"), errors="replace").read()):
            col = "B" if family(tags["Black"]) == "regicide" else "W"
            res = tags.get("Result", "").strip()
            r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
            key = (ballot_of(tags["Opening"])[1], col)
            if key in g:
                sys.exit(f"duplicate game {key} in {name}")
            g[key] = dict(res=r, moves=[m for m, _ in moves], col=col, nopen=ballot_of(tags["Opening"])[2],
                          ref=f"bl-{name}-{h} round {tags['Round']}")
    return g


def compare(x, y):
    same, diffs, who = 0, [], collections.Counter()
    for k in x:
        if k not in y:
            continue
        a, b = x[k]["moves"], y[k]["moves"]
        if a == b:
            same += 1
            continue
        i = next((i for i in range(min(len(a), len(b))) if a[i] != b[i]), min(len(a), len(b)))
        diffs.append(i + 1)
        ours = (i % 2 == 0) == (x[k]["col"] == "B")
        who["Regicide" if ours else "KingsRow"] += 1
    return same, diffs, who


def wdl(g):
    c = collections.Counter(v["res"] for v in g.values())
    return f"{c['W']} / {c['D']} / {c['L']}" + (f" ({c['?']} unfinished)" if c["?"] else "")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--results", required=True)
    ap.add_argument("--mode3", nargs="+", required=True)
    ap.add_argument("--mode1", nargs="+", required=True)
    ap.add_argument("--md")
    a = ap.parse_args()
    runs = {n: load(a.results, n) for n in a.mode3 + a.mode1}
    out = ["| Run | KingsRow book mode | games | W / D / L (Regicide) |", "|---|---|---|---|"]
    for n, g in runs.items():
        out.append(f"| {n} | {3 if n in a.mode3 else 1} | {len(g)} | {wdl(g)} |")
    out += ["", "| Runs compared | pairs | identical | first difference: median ply | first different move by Regicide / KingsRow |",
            "|---|---|---|---|---|"]
    names = list(runs)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            same, diffs, who = compare(runs[names[i]], runs[names[j]])
            n = same + len(diffs)
            out.append(f"| {names[i]} vs {names[j]} | {n} | {same} | "
                       f"{statistics.median(diffs) if diffs else '-'} | {who['Regicide']} / {who['KingsRow']} |")
    # result changes mode 1 -> mode 3, per opening and colour (all four runs)
    out += ["", "Result per opening and colour, mode 1 (both runs) against mode 3 (both runs):", "",
            "| mode 1 results | mode 3 results | openings × colours |", "|---|---|---|"]
    trans = collections.Counter()
    examples = collections.defaultdict(list)
    for k in runs[a.mode3[0]]:
        m1 = "".join(sorted(runs[n][k]["res"] for n in a.mode1 if k in runs[n]))
        m3 = "".join(sorted(runs[n][k]["res"] for n in a.mode3 if k in runs[n]))
        trans[(m1, m3)] += 1
        if "L" in m3 or ("W" in m1 and "W" not in m3) or ("W" in m3 and "W" not in m1):
            examples[(m1, m3)].append(f"{k[0]} ({'Regicide B' if k[1] == 'B' else 'Regicide W'}): "
                                      + ", ".join(runs[n][k]["ref"] for n in a.mode3))
    for (m1, m3), c in sorted(trans.items(), key=lambda kv: -kv[1]):
        out.append(f"| {m1} | {m3} | {c} |")
    out += ["", "Openings whose result changed between the modes, or with a mode-3 loss:", ""]
    for (m1, m3), ex in sorted(examples.items()):
        for e in ex:
            out.append(f"- {m1} → {m3}: {e}")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
