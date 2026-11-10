#!/usr/bin/env python3
"""Book-mode probe: does `set book N` take effect, and what does the engine's book answer in a few positions?

  book_probe.py --cbhost cbhost.exe --dll 'C:\\...\\engine.dll' [--opt 'set ...'] [--book N] [--procs 5]
                [--pos 'startpos moves ...' ...] [--repeat 2]

For each of PROCS fresh cbhost processes: the option list of the match scripts (--opt), then `set book N` if given,
`get book` (read-back), and for every position (REPEAT times, each after newgame): `go maxtime 0.5 reset`; every
command and reply is logged verbatim (raw log on stdout), followed by a summary line per process:
  SUMMARY <proc> book=<read-back> <position>: <bestmove> [<book info>] ...
"""
import argparse
import subprocess


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cbhost", required=True)
    ap.add_argument("--dll", required=True)
    ap.add_argument("--opt", action="append", default=[])
    ap.add_argument("--book")
    ap.add_argument("--procs", type=int, default=5)
    ap.add_argument("--repeat", type=int, default=2)
    ap.add_argument("--pos", action="append", default=[])
    ap.add_argument("--maxtime", default="0.5")
    ap.add_argument("--readback-only", action="store_true",
                    help="one process, options + set book, get book, no search; exit 1 unless the reply is "
                         "'reply 1 <BOOK>' (the match scripts' pre-flight check)")
    a = ap.parse_args()
    if a.readback_only:
        p = subprocess.Popen([a.cbhost, a.dll], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL, text=True, bufsize=1)
        line = ""
        while True:
            line = p.stdout.readline()
            if not line or line.startswith("ready") or line.startswith("error"):
                break
        last = ""
        for c in a.opt + [f"set book {a.book}", "get book"]:
            p.stdin.write(f"cmd {c}\n")
            p.stdin.flush()
            while True:
                line = p.stdout.readline()
                if not line or line.startswith("reply"):
                    break
            last = line.rstrip()
        p.stdin.write("quit\n")
        p.stdin.flush()
        try:
            p.wait(timeout=30)
        except subprocess.TimeoutExpired:
            p.kill()
        ok = last.split()[:3] == ["reply", "1", str(a.book)]
        print(f"book read-back: '{last}' ({'ok' if ok else 'DIFFERS'})")
        raise SystemExit(0 if ok else 1)
    positions = a.pos or ["startpos"]
    for proc in range(1, a.procs + 1):
        print(f"### process {proc}")
        p = subprocess.Popen([a.cbhost, a.dll], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                             stderr=subprocess.DEVNULL, text=True, bufsize=1)

        def send(cmd, until):
            print(f">>> {cmd}")
            p.stdin.write(cmd + "\n")
            p.stdin.flush()
            out = []
            while True:
                line = p.stdout.readline()
                if not line:
                    print("<<< (eof)")
                    return out
                print(f"<<< {line.rstrip()}")
                out.append(line.rstrip())
                if line.startswith(until):
                    return out

        while True:
            line = p.stdout.readline()
            print(f"<<< {line.rstrip()}")
            if not line or line.startswith("ready") or line.startswith("error"):
                break
        for o in a.opt:
            send(f"cmd {o}", "reply")
        if a.book is not None:
            send(f"cmd set book {a.book}", "reply")
        rb = send("cmd get book", "reply")
        readback = rb[-1] if rb else "?"
        summ = []
        for pos in positions:
            for r in range(a.repeat):
                send("newgame", "ok")
                send(f"position {pos}", "ok")
                out = send(f"go maxtime {a.maxtime} reset", "bestmove")
                best = next((l for l in out if l.startswith("bestmove")), "?")
                info = [l for l in out if "book" in l.lower()]
                summ.append(f"[{pos.replace('startpos moves ', '').replace('startpos', 'start')} #{r + 1}] "
                            f"{best.split()[1] if len(best.split()) > 1 else best}"
                            f"{' {' + info[-1][:90] + '}' if info else ''}")
        p.stdin.write("quit\n")
        p.stdin.flush()
        try:
            p.wait(timeout=30)
        except subprocess.TimeoutExpired:
            p.kill()
        print(f"SUMMARY {proc} readback=({readback}) " + " | ".join(summ))


if __name__ == "__main__":
    main()
