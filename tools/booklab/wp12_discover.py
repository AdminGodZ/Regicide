#!/usr/bin/env python3
"""WP12 stage 1 (discovery): one full game from the ballot per candidate against Cake 1.89f.

  wp12_discover.py --candidates candidates.tsv --first N [--parallel 8] [--per-lock 48] --out DIR

Candidates in rank order (candidates.tsv of wp12_select.py; rank 0 = w30/w31). For each: the candidate DLL
(wp12_dll.sh: bl-456e9b2-r11 with one book key changed, nothing else), a one-line deck "| ballot moves | A=colour"
(the colour of the line the point came from), and one game with the WP10 settings (match-cur, the current runner,
--allow-rejected-options; 0.5 s/move CheckerBoard banking; hash 128, one thread; Cake with its book, the KingsRow
2-8 WLD database, hash 128). Up to PARALLEL single-game matches run at once (the load of one -c 8 match); the
machine-wide timing lock is held for at most PER_LOCK games at a time (about 15 minutes), then released.

Every game is analysed with booklab_pinwalk on the candidate book: was the changed key reached with Regicide to
move, was the book consulted there (eligible), and was the alternative played (= consulted). Result from
Regicide's view; a win counts for nomination only if the key was consulted and the game ended on the board (not by
forfeit, illegal move, time or crash: the PDN termination/result tags and the match log are recorded).
Output: DIR/discovery.tsv (one row per attempted candidate, appended; a restart skips ranks already there).
"""
import argparse
import fcntl
import gzip
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

HOME = os.path.expanduser("~")
L = f"{HOME}/regicide-jobs/booklab"
TOOLS = os.path.dirname(os.path.abspath(__file__))
MATCH = f"{L}/bin/match-cur"
CB = "/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe"
E = r"C:\regicide-match\engines"
DB8 = r"C:\regicide-match\db\kingsrow_wld8"
RESULTS = f"{HOME}/regicide-match/results"
LOCK = f"{HOME}/regicide-jobs/locks/timing.lock"
PINWALK = f"{HOME}/regicide-booklab/build-linux/booklab_pinwalk"
EGDB = f"{HOME}/regicide-data/egdb"


def st(msg):
    with open(f"{L}/status", "a") as f:
        f.write(f"{msg} {time.strftime('%Y-%m-%dT%H:%M:%S%z')}\n")


def ballot_moves(run, rnd):
    for tags, _ in parse_pdn(open(f"{RESULTS}/{run}/games.pdn", errors="replace").read()):
        if tags["Round"] == rnd:
            return ballot_of(tags["Opening"])[1]
    raise KeyError((run, rnd))


def match_cmd(name, deck, out):
    a = f'exec:{CB} "{E}\\regicide\\bl-w12-{name}\\regicide_cb.dll"'
    b = f'exec:{CB} "{E}\\cake\\cake_189f.dll"'
    opts = ["--option-a", "set hashsize 128", "--option-a", "set threads 1"]
    for o in (f"set dbpath {DB8}", "set enable_wld 1", "set dbmbytes 512", "set max_dbpieces 8", "set hashsize 128"):
        opts += ["--option-b", o]
    return [MATCH, "--allow-rejected-options", "-a", a, "-b", b] + opts + [
        "--openings", deck, "--movetime", "0.5", "-c", "1", "--pdn-comments", "-o", out]


def analyse(c, out):
    pdn = open(f"{out}/games.pdn", errors="replace").read()
    games = parse_pdn(pdn)
    if len(games) != 1:
        return dict(result="?", reached=0, consulted=0, plies=0, term=f"{len(games)} games", off_board=True)
    tags, moves = games[0]
    col = "B" if family(tags["Black"]) == "regicide" else "W"
    res = tags.get("Result", "").strip()
    r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res, "?")
    nopen = ballot_of(tags["Opening"])[2]
    line = f"0\t{tags.get('FEN', '')}\t{col}\t{nopen}\t{' '.join(m for m, _ in moves)}\n"
    pw = subprocess.run([PINWALK, f"{L}/wp12/books/{c['name']}.bin", EGDB], input=line, capture_output=True,
                        text=True).stdout
    reached = consulted = 0
    for l in pw.splitlines():
        f = l.split("\t")
        if len(f) >= 6 and f[2] == c["fen"]:
            reached = 1
            if f[4] == "1" and f[3] == c["alt"]:
                consulted = 1
    tag = tags.get("Termination", "")
    logp = f"{out}/../{os.path.basename(out)}.log"
    log = open(logp, errors="replace").read() if os.path.exists(logp) else ""
    flags = [k for k in ("forfeit", "illegal", "crash", "timeout", "time loss", "adjudicat")
             if k in (tag + " " + log).lower()]
    return dict(result=r, reached=reached, consulted=consulted, plies=len(moves), colour=col,
                term=(tag or "-") + ("; NOT ON THE BOARD: " + ",".join(flags) if flags else ""), off_board=bool(flags))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--candidates", required=True)
    ap.add_argument("--first", type=int, required=True)
    ap.add_argument("--parallel", type=int, default=8)
    ap.add_argument("--per-lock", type=int, default=48)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    hdr, cands = None, []
    for l in open(a.candidates):
        f = l.rstrip("\n").split("\t")
        if hdr is None:
            hdr = f
            continue
        c = dict(zip(hdr, f))
        if int(c["rank"]) >= a.first:
            continue
        if c["arm"].startswith("w30/w31") and "fails" in c["arm"]:
            pass  # its own arm: still played (protocol: "as its own arm if it fails the margin"), reported apart
        c["name"] = f"c{int(c['rank']):03d}"
        c["fen"], c["alt"] = c["fen"], c["alternative"]
        cands.append(c)
    tsv = f"{a.out}/discovery.tsv"
    done = set()
    if os.path.exists(tsv):
        done = {l.split("\t")[0] for l in open(tsv)}
    else:
        with open(tsv, "w") as fo:
            fo.write("rank\tarm\tpoint\tballot\tcolour\tply\tpinned\talternative\tresult\treached\tconsulted\tplies\t"
                     "termination\tnominated\tresults_dir\n")
    todo = [c for c in cands if c["rank"] not in done]
    print(f"{len(todo)} candidates to play", flush=True)
    while todo:
        chunk, todo = todo[:a.per_lock], todo[a.per_lock:]
        # build the DLLs and decks before taking the lock
        for c in chunk:
            subprocess.run(["bash", f"{TOOLS}/wp12_dll.sh", c["name"], c["fen"], c["alt"]], check=True,
                           stdout=subprocess.DEVNULL)
            bm = ballot_moves(c["run"], c["round"])
            c["deck"] = f"{a.out}/{c['name']}.txt"
            with open(c["deck"], "w") as fo:
                fo.write(f"| {bm} | A={c['colour']}\n")
            c["outdir"] = f"{RESULTS}/bl-w12-d-{c['name']}"
        st(f"BL wait-lock w12-discovery {chunk[0]['name']}..{chunk[-1]['name']} ({len(chunk)} single games vs cake)")
        with open(LOCK, "w") as lk:
            fcntl.flock(lk, fcntl.LOCK_EX)
            st(f"BL start w12-discovery {chunk[0]['name']}..{chunk[-1]['name']}")
            queue, running = list(chunk), []
            while queue or running:
                while queue and len(running) < a.parallel:
                    c = queue.pop(0)
                    subprocess.run(["rm", "-rf", c["outdir"]])
                    logf = open(f"{RESULTS}/bl-w12-d-{c['name']}.log", "w")
                    p = subprocess.Popen(match_cmd(c["name"], c["deck"], c["outdir"]), stdout=logf,
                                         stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
                    running.append((p, c, logf))
                time.sleep(2)
                for p, c, logf in list(running):
                    if p.poll() is not None:
                        logf.close()
                        running.remove((p, c, logf))
                        c["rc"] = p.returncode
            fcntl.flock(lk, fcntl.LOCK_UN)
        st(f"BL end w12-discovery {chunk[0]['name']}..{chunk[-1]['name']}")
        with open(tsv, "a") as fo:
            for c in chunk:
                try:
                    r = analyse(c, c["outdir"])
                except Exception as ex:  # noqa: BLE001
                    r = dict(result="?", reached=0, consulted=0, plies=0, term=f"error {ex} rc={c.get('rc')}",
                             off_board=True)
                nom = int(r["result"] == "W" and r["consulted"] == 1 and not r.get("off_board", True))
                fo.write(f"{c['rank']}\t{c['arm']}\t{c['point']}\t{c['ballot']}\t{c['colour']}\t{c['ply']}\t"
                         f"{c['pinned']}\t{c['alt']}\t{r['result']}\t{r['reached']}\t{r['consulted']}\t{r['plies']}\t"
                         f"{r['term']}\t{nom}\t{c['outdir']}\n")
        print(f"chunk done: {chunk[0]['name']}..{chunk[-1]['name']}", flush=True)


if __name__ == "__main__":
    main()
