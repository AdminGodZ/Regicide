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


def shim_rows(d, played=None):
    """(go wall interval, position+go interval, host S) per search. With `played` (a set of move-list prefixes of
    positions where an engine moved in the games), only searches of those positions count: the runner's warm-up
    search at every engine start (review 2026-11-10 d) and anything else not played are left out."""
    rows = []
    for f in sorted(os.listdir(d)):
        t_pos = t_go = pos = None
        for l in open(os.path.join(d, f), errors="replace"):
            t, direc, line = l.rstrip("\n").split("\t", 2)
            t = float(t)
            if direc == ">" and line.startswith("position"):
                t_pos = t
                pos = position_key(line)
            elif direc == ">" and line.startswith("go"):
                t_go = t
            elif direc == "<" and line.startswith("bestmove") and t_go is not None:
                m = re.search(r"\btime ([0-9.]+)", line)
                if m and (played is None or pos in played):
                    rows.append((t - t_go, t - (t_pos if t_pos is not None else t_go), float(m.group(1))))
                t_go = t_pos = None
    return rows


def position_key(line):
    """(initial board, move sequence) of a host 'position' command: 'position startpos [moves ...]' or
    'position fen FEN [moves ...]' (review 2026-11-11 A3: games started from a FEN without ballot moves)."""
    rest = line.strip()[len("position"):].strip()
    head, _, moves = (rest + " ").partition(" moves ")
    head = head.strip()
    board = "start" if head in ("startpos", "") else (head[4:].strip() if head.startswith("fen ") else head)
    return board, moves.strip()


def played_positions(run_dir):
    """(initial board, move sequence) of every position where an engine (not a dealt ballot move) moved in the
    run's games; the initial board is the game's FEN tag, or the start position."""
    out = set()
    for tags, moves in parse_pdn(open(os.path.join(run_dir, "games.pdn"), errors="replace").read()):
        mv = [m for m, _ in moves]
        board = tags.get("FEN", "").strip() or "start"
        nopen = 0
        o = tags.get("Opening", "")
        if ": " in o:
            nopen = len(o.rsplit(": ", 1)[1].split())
        for i in range(nopen, len(mv)):
            out.add((board, " ".join(mv[:i])))
    return out


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
           "line (with the game history) instead. Runner charge: the time in the PDN comment (2 decimals). Only "
           "searches of positions played in the games count (the runner's warm-up search at every engine start "
           "is left out).", "",
           "| Engine (match) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max | host S mean |",
           "|---|---|---|---|---|"]
    per_engine = {}
    for opp in ("cake", "kingsrow", "guinn"):
        run = next((r for r in runs if os.path.basename(r.rstrip("/")).endswith(opp)), None)
        played = played_positions(run) if run else None
        for side, eng in (("A", "Regicide"), ("B", names[opp])):
            d = os.path.join(acct, opp, side)
            if not os.path.isdir(d):
                continue
            rows = [(w * rate, pw * rate, hs) for w, pw, hs in shim_rows(d, played)]
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
