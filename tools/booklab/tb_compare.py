#!/usr/bin/env python3
"""Test bed for the #6 Black repair (lead 2026-11-11 12:35): repeats of the deck prefix (games 1-10) against the
reference run.

  tb_compare.py --ref RUNDIR --reps RUNDIR ... [--game 9] [--ply 50] [--md OUT]
Per repeat: are games 1 .. GAME-1 move-identical to the reference's games 1 .. GAME-1 (and if not, the first game and
ply that differ, which side moved, both moves); the opponent's move at PLY of game GAME, and the result of that game.
"""
import argparse
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn  # noqa: E402


def games(d, n):
    out = {}
    for p in sorted(glob.glob(os.path.join(d, "games", "*.pdn")))[:n]:
        for t, m in parse_pdn(open(p).read()):
            out[int(os.path.basename(p)[:5])] = (t, [x for x, _ in m])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--ref", required=True)
    ap.add_argument("--reps", nargs="+", required=True)
    ap.add_argument("--game", type=int, default=9)
    ap.add_argument("--ply", type=int, default=50)
    ap.add_argument("--md")
    a = ap.parse_args()
    ref = games(a.ref, a.game + 1)
    out = ["| Repeat | games 1-%d identical to the reference | first difference | game %d follows the reference game to ply %d | "
           "game %d: opponent at ply %d | game %d result |" % (a.game - 1, a.game, a.ply - 1, a.game, a.ply, a.game),
           "|---|---|---|---|---|---|"]
    for d in a.reps:
        g = games(d, a.game + 1)
        diff = None
        for k in range(1, a.game + 1):
            if k not in g:
                diff = f"game {k} missing"
                break
            r, x = ref[k][1], g[k][1]
            i = next((i for i in range(min(len(r), len(x))) if r[i] != x[i]), None if len(r) == len(x) else min(len(r), len(x)))
            if i is not None:
                rb = ref[k][0]["Black"].lower().startswith("regicide")
                side = "Regicide" if (i % 2 == 0) == rb else "opponent"
                diff = (k, f"game {k} ply {i + 1} ({side}): {x[i] if i < len(x) else 'end'} (reference {r[i] if i < len(r) else 'end'})")
                break
        same = "yes" if diff is None or (isinstance(diff, tuple) and diff[0] >= a.game) else "no"
        t9, m9 = g.get(a.game, ({}, []))
        res = t9.get("Result", "?")
        rb9 = t9.get("Black", "").lower().startswith("regicide")
        our = {"1-0": "W" if rb9 else "L", "0-1": "L" if rb9 else "W", "1/2-1/2": "D"}.get(res, "?")
        r9 = ref[a.game][1]
        k9 = next((i for i in range(min(len(r9), len(m9))) if r9[i] != m9[i]), None)
        pre = "yes" if m9 and (k9 is None or k9 >= a.ply - 1) else (f"no, from ply {k9 + 1}" if m9 else "—")
        out.append(f"| {os.path.basename(d)} | {same} | {diff[1] if isinstance(diff, tuple) else (diff or '— (games 1-%d identical)' % a.game)} | "
                   f"{pre} | {m9[a.ply - 1] if len(m9) >= a.ply else '—'} | {our} |")
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
