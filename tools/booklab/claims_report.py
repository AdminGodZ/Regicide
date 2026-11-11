#!/usr/bin/env python3
"""Claims census (lead 2026-11-11 11:57): what CheckerBoard's claim handling (--cb-claims) would change.

  claims_report.py RUNDIR ... [--md OUT]
Runner 8de39d0 records every claim an engine returns from getmove: PDN tags FirstClaim ("ply 37 B draw": ply, the
claimant's colour, the claim), ClaimsA / ClaimsB ("w0 l0 d12": win, loss, draw claims of engine A = Regicide and B =
the opponent), and claim=... in the move comments. Per opponent: games with at least one claim of each kind by each
side; the first claim's ply and piece count (24 minus the pieces captured before it); and every game whose first claim
disagrees with the result on the board (a draw claim in a decisive game, a win claim the claimant did not win, a loss
claim the claimant did not lose), with opening, our colour, ply, claimant, claim and result. With --cb-claims such a
game would have ended at that claim with the claimed result.
"""
import argparse
import collections
import os
import re
import statistics
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, ballot_of, family  # noqa: E402


def counts(s):
    m = re.match(r"w(\d+) l(\d+) d(\d+)", s or "")
    return dict(win=int(m.group(1)), loss=int(m.group(2)), draw=int(m.group(3))) if m else None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("runs", nargs="+")
    ap.add_argument("--md")
    a = ap.parse_args()
    per = collections.defaultdict(lambda: dict(games=0, side=collections.Counter(), first=[], bad=[], noclaim=0,
                                                firstkind=collections.Counter()))
    for d in a.runs:
        for tags, moves in parse_pdn(open(os.path.join(d, "games.pdn"), errors="replace").read()):
            rb = tags["Black"].lower().startswith("regicide")
            opp = family(tags["White"] if rb else tags["Black"])
            P = per[opp]
            P["games"] += 1
            ca, cb = counts(tags.get("ClaimsA")), counts(tags.get("ClaimsB"))
            if ca is None or cb is None:
                sys.exit(f"{d} round {tags.get('Round')}: no ClaimsA/ClaimsB tags (runner before 8de39d0?)")
            for who, c in (("Regicide", ca), (opp, cb)):
                for k, n in c.items():
                    if n:
                        P["side"][(who, k)] += 1
            # every decisive game: did the opponent claim a draw (or its own win) before the end? (lead 12:35: such a
            # win of ours would not exist under CheckerBoard's handling; likewise for a loss)
            res0 = {"1-0": "B", "0-1": "W"}.get(tags.get("Result", "").strip())
            if res0:
                our = "B" if rb else "W"
                oc = [(i + 1, re.search(r"claim=(\w+)", c or "").group(1)) for i, (_, c) in enumerate(moves)
                      if (i % 2 == 0) != rb and re.search(r"claim=(\w+)", c or "")]
                rc = [(i + 1, re.search(r"claim=(\w+)", c or "").group(1)) for i, (_, c) in enumerate(moves)
                      if (i % 2 == 0) == rb and re.search(r"claim=(\w+)", c or "")]
                fo = next((f"ply {p} {k}" for p, k in oc), "none")
                fr = next((f"ply {p} {k}" for p, k in rc), "none")
                P.setdefault("decisive", []).append(
                    f"| {opp} | {ballot_of(tags['Opening'])[0]} | {our} | {os.path.basename(d)} {tags.get('Round')} | "
                    f"{'Regicide won' if res0 == our else opp + ' won'} ({len(moves)} plies) | {fo} | {fr} | "
                    f"{'yes' if any(k == 'draw' for _, k in oc) else 'no'} |")
            fc = tags.get("FirstClaim", "")
            m = re.match(r"ply (\d+) ([BW]) (win|loss|draw)", fc)
            if not m:
                P["noclaim"] += 1
                continue
            ply, ccol, kind = int(m.group(1)), m.group(2), m.group(3)
            who = "Regicide" if (ccol == "B") == rb else opp
            pieces = 24 - sum(mv.count("x") for mv, _ in moves[:ply - 1])
            P["first"].append((ply, pieces))
            P["firstkind"][(who, kind)] += 1
            res = tags.get("Result", "").strip()   # 1-0 = Black won (this runner's PDN convention)
            board = {"1-0": "B", "0-1": "W", "1/2-1/2": "D"}.get(res, "?")
            bad = (kind == "draw" and board in "BW") or (kind == "win" and board != ccol) or \
                  (kind == "loss" and board not in ("W" if ccol == "B" else "B"))
            if bad:
                our = "B" if rb else "W"
                ours = {"D": "draw", our: "Regicide won"}.get(board, f"{opp} won")
                P["bad"].append(f"| {opp} | {ballot_of(tags['Opening'])[0]} | {our} | {os.path.basename(d)} {tags.get('Round')} | "
                                f"ply {ply} ({pieces} pieces) | {who}: {kind} | {ours} ({len(moves)} plies) |")
    out = []
    for opp, P in sorted(per.items()):
        f = P["first"]
        out += [f"## {opp}: {P['games']} games", "",
                "Games with at least one claim: " + ", ".join(f"{w} {k} {n}" for (w, k), n in sorted(P["side"].items())) +
                f"; games without any claim {P['noclaim']}.",
                "First claim of a game: " + ", ".join(f"{w} {k} {n}" for (w, k), n in sorted(P["firstkind"].items())) +
                (f"; ply median {statistics.median(p for p, _ in f):.0f} (range {min(p for p, _ in f)}-{max(p for p, _ in f)}), "
                 f"pieces median {statistics.median(q for _, q in f):.0f} (range {min(q for _, q in f)}-{max(q for _, q in f)})" if f else ""),
                f"First claim disagrees with the result on the board: **{len(P['bad'])}**", ""]
        if P["bad"]:
            out += ["| Opponent | Opening | Regicide | Game | first claim at | claimant: claim | result on the board |",
                    "|---|---|---|---|---|---|---|"] + P["bad"] + [""]
        if P.get("decisive"):
            out += ["Decisive games and the claims in them (the opponent's first claim, ours, and whether the opponent "
                    "claimed a draw before the end: then CheckerBoard would have scored a draw):", "",
                    "| Opponent | Opening | Regicide | Game | result | opponent's first claim | our first claim | "
                    "opponent claimed a draw? |", "|---|---|---|---|---|---|---|---|"] + P["decisive"] + [""]
    text = "\n".join(out)
    print(text)
    if a.md:
        open(a.md, "w").write(text + "\n")


if __name__ == "__main__":
    main()
