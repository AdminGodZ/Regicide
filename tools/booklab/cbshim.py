#!/usr/bin/env python3
"""Logging shim between the match runner and cbhost (review item C: what did the runner's wall-time accounting cost
each engine?). Engine spec: exec:python3 cbshim.py LOGDIR cbhost.exe "C:\\...\\engine.dll"

Passes every line through unchanged in both directions and appends to LOGDIR/<pid>.tsv one row per line:
  monotonic time (s) <TAB> direction (> runner to host, < host to runner) <TAB> line
The analysis (cbshim_report.py) pairs each "go ..." with the next "bestmove ... time S" and compares the interval
seen here, and the runner's own figure in the PDN comment, with the host's S.
"""
import os
import subprocess
import sys
import threading
import time


def main():
    logdir, cmd = sys.argv[1], sys.argv[2:]
    os.makedirs(logdir, exist_ok=True)
    log = open(os.path.join(logdir, f"{os.getpid()}.tsv"), "w", buffering=1)
    lock = threading.Lock()
    p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, bufsize=0)

    def rec(d, line):
        with lock:
            log.write(f"{time.monotonic():.6f}\t{d}\t{line.decode(errors='replace').rstrip()}\n")

    def up():  # host -> runner
        out = sys.stdout.buffer
        for line in iter(p.stdout.readline, b""):
            out.write(line)
            out.flush()
            rec("<", line)
        out.close()

    t = threading.Thread(target=up, daemon=True)
    t.start()
    inp = sys.stdin.buffer
    for line in iter(inp.readline, b""):
        rec(">", line)
        p.stdin.write(line)
        p.stdin.flush()
    try:
        p.stdin.close()
    except OSError:
        pass
    p.wait()
    t.join(timeout=5)


if __name__ == "__main__":
    main()
