#!/usr/bin/env python3
"""Opponent configuration evidence: drive one CheckerBoard engine through cbhost, send the option list the match
runner sends, and log every command and reply verbatim; then query settings and run two searches.

  engine_probe.py --cbhost cbhost.exe --dll 'C:\\...\\engine.dll' --opt 'set ...' [--opt ...] [--get 'get book' ...]
"""
import argparse
import subprocess
import sys
import time


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cbhost", required=True)
    ap.add_argument("--dll", required=True)
    ap.add_argument("--opt", action="append", default=[])
    ap.add_argument("--get", action="append", default=[])
    ap.add_argument("--fen", action="append", default=[])
    ap.add_argument("--maxtime", default="0.5")
    a = ap.parse_args()
    p = subprocess.Popen([a.cbhost, a.dll], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                         text=True, bufsize=1)

    def send(cmd, until):
        print(f">>> {cmd}")
        p.stdin.write(cmd + "\n")
        p.stdin.flush()
        while True:
            line = p.stdout.readline()
            if not line:
                print("<<< (eof)")
                return
            print(f"<<< {line.rstrip()}")
            if line.startswith(until):
                return

    # startup banner
    while True:
        line = p.stdout.readline()
        print(f"<<< {line.rstrip()}")
        if not line or line.startswith("ready") or line.startswith("error"):
            break
    for o in a.opt:
        send(f"cmd {o}", "reply")
    for g in a.get:
        send(f"cmd {g}", "reply")
    send("newgame", "ok")
    send("position startpos", "ok")
    t = time.time()
    send(f"go maxtime {a.maxtime} reset", "bestmove")
    print(f"(start position: {time.time() - t:.2f} s wall)")
    for fen in a.fen:
        send("newgame", "ok")
        send(f"position fen {fen}", "ok")
        t = time.time()
        send(f"go maxtime {a.maxtime} reset", "bestmove")
        print(f"({fen}: {time.time() - t:.2f} s wall)")
    # engines that initialise lazily (book, database, nets on the first search): ask again afterwards
    for g in a.get:
        if g in ("about", "get book", "get max_dbpieces", "get dbpieces"):
            send(f"cmd {g}", "reply")
    # a second game from the start position: some engines load their book only on the first search
    send("newgame", "ok")
    send("position startpos moves 11-15", "ok")
    send(f"go maxtime {a.maxtime} reset", "bestmove")
    p.stdin.write("quit\n")
    p.stdin.flush()
    p.wait(timeout=30)


if __name__ == "__main__":
    main()
