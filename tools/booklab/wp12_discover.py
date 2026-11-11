#!/usr/bin/env python3
"""WP12 stage 1 (discovery): one full game from the ballot per candidate against Cake 1.89f.

  wp12_discover.py --candidates DIR/candidates.tsv --first N [--parallel 8] [--per-lock 48] --out DIR
  env MATCH (runner), CBHOST (cbhost.exe path): default the PR #49 builds (bin/match-pr49, bin/pr49/cbhost.exe)

Candidates in rank order (wp12_select.py; rank 0 = the known position). For each: the candidate DLL (wp12_dll.sh:
bl-456e9b2-r11 with one book key changed, nothing else), a one-line deck "| ballot moves | A=colour" (the ballot and
colour of the Cake game the point came from), and one game: 0.5 s/move CheckerBoard banking, hash 128, one thread,
`set book 3` for both engines (read back before every chunk, with each chunk's first candidate DLL and Cake), Cake with
its book and the KingsRow 2-8 WLD database. Up to PARALLEL single-game matches at once (the load of one -c 8 match);
the timing lock is held for at most PER_LOCK games at a time. Before each chunk Windows free commit must be >= 14 GB
(else the WSL page cache is dropped, as in bl_run_w.sh).

Ledger (DIR/discovery.tsv) is bound to the candidate list: its first line holds the SHA-256 of candidates.tsv, and a
resume with a different list stops. A game is entered in the ledger only after a complete analysis (booklab_pinwalk
on the candidate book: was the changed key reached with Regicide to move, consulted, and the alternative played);
if the analysis fails the game is written to DIR/quarantine.tsv and analysed again at the next start (review M1),
never marked done. A win nominates the candidate only if the key was consulted and the game ended on the board (no
crash, time-out, illegal move or engine that did not start).
"""
import argparse
import fcntl
import hashlib
import json
import os
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from theory import parse_pdn, family, ballot_of  # noqa: E402

HOME = os.path.expanduser("~")
L = f"{HOME}/regicide-jobs/booklab"
TOOLS = os.path.dirname(os.path.abspath(__file__))
MATCH = os.environ.get("MATCH", f"{L}/bin/match-pr49")
CB = os.environ.get("CBHOST", f"{L}/bin/pr49/cbhost.exe")
E = r"C:\regicide-match\engines"
DB8 = r"C:\regicide-match\db\kingsrow_wld8"
RESULTS = f"{HOME}/regicide-match/results"
LOCK = f"{HOME}/regicide-jobs/locks/timing.lock"
PINWALK = f"{HOME}/regicide-booklab/build-linux/booklab_pinwalk"
EGDB = f"{HOME}/regicide-data/egdb"
PROBE = f"{TOOLS}/book_probe.py"
PS = "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
OFF_BOARD = ("crashed", "did not start", "timed out", "illegal", "protocol error", "forfeit", "time loss")
CAKE_OPTS = [f"set dbpath {DB8}", "set enable_wld 1", "set dbmbytes 512", "set max_dbpieces 8", "set hashsize 128"]


def st(msg):
    with open(f"{L}/status", "a") as f:
        f.write(f"{msg} {time.strftime('%Y-%m-%dT%H:%M:%S%z')}\n")


def sha(path):
    return hashlib.sha256(open(path, "rb").read()).hexdigest()


def ballot_moves(run, rnd):
    for tags, _ in parse_pdn(open(f"{RESULTS}/{run}/games.pdn", errors="replace").read()):
        if tags["Round"] == rnd:
            return ballot_of(tags["Opening"])[1]
    raise KeyError((run, rnd))


def dll(name):
    return f"{E}\\regicide\\bl-w12-{name}\\regicide_cb.dll"


def match_cmd(name, deck, out):
    opts = ["--option-a", "set hashsize 128", "--option-a", "set threads 1", "--option-a", "set book 3"]
    for o in CAKE_OPTS + ["set book 3"]:
        opts += ["--option-b", o]
    return [MATCH, "--allow-rejected-options", "-a", f'exec:{CB} "{dll(name)}"',
            "-b", f'exec:{CB} "{E}\\cake\\cake_189f.dll"'] + opts + [
        "--openings", deck, "--movetime", "0.5", "-c", "1", "--pdn-comments", "-o", out]


def readback(dllpath, opts):
    args = ["python3", PROBE, "--cbhost", CB, "--dll", dllpath, "--book", "3", "--readback-only"]
    for o in opts:
        args += ["--opt", o]
    r = subprocess.run(args, capture_output=True, text=True)
    return r.returncode == 0, r.stdout.strip()


def commit_free():
    r = subprocess.run([PS, "-NoProfile", "-Command",
                        "[math]::Round((Get-CimInstance Win32_OperatingSystem).FreeVirtualMemory/1MB,1)"],
                       capture_output=True, text=True)
    try:
        return float(r.stdout.strip())
    except ValueError:
        return -1.0


def ensure_memory(minimum=14.0):
    c, t, text = commit_free(), 0, ""
    text = f"commit free {c} GB"
    while c < minimum and t < 600:
        subprocess.run(["/mnt/c/Windows/System32/wsl.exe", "-u", "root", "--", "sh", "-c",
                        "sync; echo 1 > /proc/sys/vm/drop_caches"], capture_output=True)
        time.sleep(30)
        t += 30
        c = commit_free()
        text += f"; page cache dropped, {c} GB after {t} s"
    return text


def analyse(c, out):
    """Full analysis or an exception (the caller quarantines)."""
    games = list(parse_pdn(open(f"{out}/games.pdn", errors="replace").read()))
    if len(games) != 1:
        raise ValueError(f"{len(games)} games in {out}")
    tags, moves = games[0]
    col = "B" if family(tags["Black"]) == "regicide" else "W"
    if col != c["colour"]:
        raise ValueError(f"Regicide played {col}, the candidate is for {c['colour']}")
    res = tags.get("Result", "").strip()
    r = {"1-0": "W" if col == "B" else "L", "0-1": "W" if col == "W" else "L", "1/2-1/2": "D"}.get(res)
    if r is None:
        raise ValueError(f"no final result ({res!r})")
    nopen = ballot_of(tags["Opening"])[2]
    line = f"0\t{tags.get('FEN', '')}\t{col}\t{nopen}\t{' '.join(m for m, _ in moves)}\n"
    pw = subprocess.run([PINWALK, f"{L}/wp12/books/{c['name']}.bin", EGDB], input=line, capture_output=True,
                        text=True, check=True).stdout
    if not pw.strip() or "\tERR\t" in pw:
        raise ValueError(f"pinwalk: {pw.strip()[:200]}")
    reached = consulted = 0
    for l in pw.splitlines():
        f = l.split("\t")
        if f[2] == c["fen"]:
            reached = 1
            consulted |= int(f[4] == "1" and f[3] == c["alt"])
    tag = tags.get("Termination", "")
    flags = [k for k in OFF_BOARD if k in tag.lower()]
    return dict(result=r, reached=reached, consulted=consulted, plies=len(moves),
                term=(tag or "-") + ("; NOT ON THE BOARD" if flags else ""), off_board=bool(flags))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--candidates", required=True)
    ap.add_argument("--first", type=int, required=True)
    ap.add_argument("--parallel", type=int, default=8)
    ap.add_argument("--per-lock", type=int, default=48)
    ap.add_argument("--out", required=True)
    ap.add_argument("--replay", nargs="*", default=[], help="ranks whose saved game is archived and played again")
    ap.add_argument("--base-dll", default="bl-456e9b2-r11k", help="engine folder of the baseline (r11) DLL")
    ap.add_argument("--base-book", default=f"{L}/book_r11k.bin", help="the baseline book embedded in that DLL")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    csha = sha(a.candidates)
    camp = open(a.candidates).readline().strip()
    if not camp.startswith("# campaign "):
        sys.exit(f"{a.candidates}: first line must be '# campaign KEY' (wp12_select.py)")
    camp8 = camp.split()[2][:8]
    base_dll = f"/mnt/c/regicide-match/engines/regicide/{a.base_dll}/regicide_cb.dll"
    # what every saved game of this campaign must have been played with (checked before a saved game is imported)
    manifest = dict(campaign=camp.split()[2], candidates_sha256=csha, base_dll=a.base_dll, base_dll_sha256=sha(base_dll),
                    base_book_sha256=sha(a.base_book), runner_sha256=sha(MATCH), host_sha256=sha(CB),
                    cake_sha256=sha("/mnt/c/regicide-match/engines/cake/cake_189f.dll"))
    os.environ["W12_BASE_DLL"], os.environ["W12_BASE_BOOK"] = a.base_dll, a.base_book
    hdr, cands = None, []
    for l in open(a.candidates):
        if l.startswith("#"):
            continue
        f = l.rstrip("\n").split("\t")
        if hdr is None:
            hdr = f
            continue
        c = dict(zip(hdr, f))
        if int(c["rank"]) >= a.first:
            continue
        # results, candidate books and DLLs carry the campaign key (review 2026-11-11 A1), so a new campaign can never
        # meet the saved games of an older one under the same name
        c["name"] = f"{camp8}-c{int(c['rank']):03d}"
        c["alt"] = c["alternative"]
        c["outdir"] = f"{RESULTS}/bl-w12-{c['name']}"
        cands.append(c)
    tsv, qtsv = f"{a.out}/discovery.tsv", f"{a.out}/quarantine.tsv"
    mline = "# manifest " + json.dumps(manifest, sort_keys=True)
    if os.path.exists(tsv):
        with open(tsv) as fi:
            first, second = fi.readline().strip(), fi.readline().strip()
        if first != f"# candidates sha256 {csha}":
            sys.exit(f"ledger {tsv} belongs to another candidate list ({first}); use a new --out")
        # the rows of a ledger count only for the runtime they were played with (review 2026-11-11): baseline DLL and
        # book, runner, host and opponent must be the same as now, otherwise nothing is resumed
        if second != mline:
            old = json.loads(second[len("# manifest "):]) if second.startswith("# manifest ") else {}
            diff = sorted(k for k in set(old) | set(manifest) if old.get(k) != manifest.get(k))
            sys.exit(f"ledger {tsv} was written with a different runtime ({', '.join(diff) or 'no manifest line'}); "
                     "use a new --out")
    else:
        with open(tsv, "w") as fo:
            fo.write(f"# candidates sha256 {csha}\n{mline}\n")
            fo.write("rank\tarm\tpoint\tballot\tcolour\tply\tpinned\talternative\tresult\treached\tconsulted\tplies\t"
                     "termination\tnominated\tresults_dir\n")
    done = {l.split("\t")[0] for l in open(tsv) if not l.startswith("#")}

    def record(chunk):
        quarantined = []
        with open(tsv, "a") as fo:
            for c in chunk:
                try:
                    r = analyse(c, c["outdir"])
                except Exception as ex:  # noqa: BLE001
                    quarantined.append(f"{c['rank']}\t{c['outdir']}\t{type(ex).__name__}: {ex}\t"
                                       f"{time.strftime('%Y-%m-%dT%H:%M:%S')}")
                    continue
                nom = int(r["result"] == "W" and r["consulted"] == 1 and not r["off_board"])
                fo.write(f"{c['rank']}\t{c['arm']}\t{c['point']}\t{c['ballot']}\t{c['colour']}\t{c['ply']}\t"
                         f"{c['pinned']}\t{c['alt']}\t{r['result']}\t{r['reached']}\t{r['consulted']}\t{r['plies']}\t"
                         f"{r['term']}\t{nom}\t{c['outdir']}\n")
                done.add(c["rank"])
        if quarantined:
            with open(qtsv, "a") as fo:
                fo.write("\n".join(quarantined) + "\n")
            st(f"W12 quarantined {len(quarantined)} games (analysis failed; see {qtsv})")

    # An intended replay (--replay RANK) archives the saved game first; nothing else is ever deleted.
    for c in cands:
        if c["rank"] in a.replay and os.path.exists(c["outdir"]):
            arch = f"{c['outdir']}.replaced-{time.strftime('%Y%m%dT%H%M%S')}"
            os.rename(c["outdir"], arch)
            st(f"W12 replay of {c['name']} requested: saved game archived as {arch}")
    # games played before but not analysed (quarantined, or interrupted after play): analyse again, never replay; a
    # saved game is imported only if its manifest matches this campaign (baseline, engine, runner, host, deck)
    saved, foreign = [], []
    for c in cands:
        if c["rank"] in done or not (os.path.exists(f"{c['outdir']}/games.pdn") and os.path.exists(f"{c['outdir']}/summary.txt")):
            continue
        want = dict(manifest, rank=c["rank"], fen=c["fen"], alternative=c["alt"])
        mpath = f"{c['outdir']}/w12_manifest.json"
        have = json.load(open(mpath)) if os.path.exists(mpath) else None
        if have is not None and {k: have.get(k) for k in want} == want:
            saved.append(c)
        else:
            foreign.append(c)
    if foreign:
        st(f"W12 {len(foreign)} saved games under this campaign's names have no matching manifest; kept, not imported, "
           f"not replayed (first: {foreign[0]['outdir']})")
    record(saved)
    held_foreign = {c["rank"] for c in foreign}
    held = {c["rank"] for c in saved if c["rank"] not in done}  # analysis still failing: kept, not in the play queue
    if held:
        st(f"W12 {len(held)} saved games still fail analysis and stay out of the play queue (quarantine.tsv; "
           f"resolve, or replay explicitly with --replay RANK)")
    todo = [c for c in cands if c["rank"] not in done and c["rank"] not in held and c["rank"] not in held_foreign]
    print(f"{len(todo)} candidates to play", flush=True)
    while todo:
        chunk, todo = todo[:a.per_lock], todo[a.per_lock:]
        for c in chunk:  # DLLs and decks before taking the lock
            subprocess.run(["bash", f"{TOOLS}/wp12_dll.sh", c["name"], c["fen"], c["alt"]], check=True,
                           stdout=subprocess.DEVNULL)
            c["deck"] = f"{a.out}/{c['name']}.txt"
            with open(c["deck"], "w") as fo:
                fo.write(f"| {ballot_moves(c['run'], c['round'])} | A={c['colour']}\n")
        ok_a, ra = readback(dll(chunk[0]["name"]), ["set hashsize 128", "set threads 1"])
        ok_b, rb = readback(f"{E}\\cake\\cake_189f.dll", CAKE_OPTS)
        if not (ok_a and ok_b):
            st(f"W12 refused: book read-back differs: A {ra}; B {rb}")
            sys.exit(3)
        names = f"{chunk[0]['name']}..{chunk[-1]['name']}"
        st(f"BL wait-lock w12-discovery {names} ({len(chunk)} single games vs cake)")
        with open(LOCK, "w") as lk:
            fcntl.flock(lk, fcntl.LOCK_EX)
            mem = ensure_memory()
            st(f"BL start w12-discovery {names}; A {ra}; B {rb}; {mem}; runner {MATCH}; host {CB}")
            queue, running = list(chunk), []
            while queue or running:
                while queue and len(running) < a.parallel:
                    c = queue.pop(0)
                    if os.path.exists(c["outdir"]):  # an unfinished earlier attempt (no summary): archived, not deleted
                        os.rename(c["outdir"], f"{c['outdir']}.partial-{time.strftime('%Y%m%dT%H%M%S')}")
                    logf = open(f"{RESULTS}/bl-w12-{c['name']}.log", "w")
                    p = subprocess.Popen(match_cmd(c["name"], c["deck"], c["outdir"]), stdout=logf,
                                         stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL)
                    c["manifest"] = dict(manifest, rank=c["rank"], fen=c["fen"], alternative=c["alt"],
                                         deck=open(c["deck"]).read().strip(),
                                         dll_sha256=sha(f"/mnt/c/regicide-match/engines/regicide/bl-w12-{c['name']}/regicide_cb.dll"))
                    running.append((p, c, logf))
                time.sleep(2)
                for p, c, logf in list(running):
                    if p.poll() is not None:
                        logf.close()
                        running.remove((p, c, logf))
                        c["rc"] = p.returncode
                        os.makedirs(c["outdir"], exist_ok=True)
                        json.dump(c["manifest"], open(f"{c['outdir']}/w12_manifest.json", "w"), indent=1)
            fcntl.flock(lk, fcntl.LOCK_UN)
        st(f"BL end w12-discovery {names}")
        record(chunk)
        print(f"chunk done: {names}", flush=True)


if __name__ == "__main__":
    main()
