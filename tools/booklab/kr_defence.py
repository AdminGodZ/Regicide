#!/usr/bin/env python3
"""Ask an engine (KingsRow in book mode 3) for its move at each of Regicide's positions on lost lines.

  kr_defence.py --cbhost cbhost.exe --dll 'C:\\...\\Kingsrow64.dll' [--opt 'set ...'] --book 3 --maxtime 1
                --game PDN:ROUND ... [--plies 30]
For each game, for each position where Regicide is to move from the end of the ballot up to PLIES plies: the
engine's answer (`position startpos moves ...`, `go maxtime T reset`), every info line it prints, and Regicide's move
in the game. Output TSV: game, ply (one-based), Regicide's move, engine move, same?, last info line.
"""
import argparse
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cbhost", required=True)
    ap.add_argument("--dll", required=True)
    ap.add_argument("--opt", action="append", default=[])
    ap.add_argument("--book", default="3")
    ap.add_argument("--maxtime", default="1")
    ap.add_argument("--plies", type=int, default=30)
    ap.add_argument("--game", action="append", required=True)
    a = ap.parse_args()
    p = subprocess.Popen([a.cbhost, a.dll], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                         text=True, bufsize=1)

    def send(cmd, until):
        p.stdin.write(cmd + "\n")
        p.stdin.flush()
        out = []
        while True:
            line = p.stdout.readline()
            if not line:
                return out
            out.append(line.rstrip())
            if line.startswith(until):
                return out

    while True:
        line = p.stdout.readline()
        if not line or line.startswith("ready") or line.startswith("error"):
            break
    for o in a.opt:
        send(f"cmd {o}", "reply")
    rb = send(f"cmd set book {a.book}", "reply")
    print(f"# set book {a.book}: {rb[-1] if rb else '?'}")
    print("game\tply\tregicide\tengine\tsame\tinfo")
    for spec in a.game:
        path, rnd = spec.rsplit(":", 1)
        for tags, moves in parse_pdn(open(path, errors="replace").read()):
            if tags["Round"] != rnd:
                continue
            col = "B" if family(tags["Black"]) == "regicide" else "W"
            nopen = ballot_of(tags["Opening"])[2]
            mv = [m for m, _ in moves]
            name = f"{os.path.basename(os.path.dirname(path))} r{rnd} {ballot_of(tags['Opening'])[0]}"
            for i in range(nopen, min(len(mv), nopen + a.plies)):
                if (i % 2 == 0) != (col == "B"):
                    continue
                send("newgame", "ok")
                send("position startpos" + (" moves " + " ".join(mv[:i]) if i else ""), "ok")
                out = send(f"go maxtime {a.maxtime} reset", "bestmove")
                best = next((l.split()[1] for l in out if l.startswith("bestmove")), "?")
                info = [l for l in out if l.startswith("info")]
                print(f"{name}\t{i + 1}\t{mv[i]}\t{best}\t{int(best == mv[i])}\t{info[-1][:150] if info else ''}",
                      flush=True)
    p.stdin.write("quit\n")
    p.stdin.flush()
    p.wait(timeout=30)


if __name__ == "__main__":
    main()
