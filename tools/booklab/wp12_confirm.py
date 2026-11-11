#!/usr/bin/env python3
"""WP12 stages 2 and 3 (PROTOCOL.md on master): runner and evaluator for one candidate.

  wp12_confirm.py stage2 --tag T --cand-dll DIR --cand-book BOOK --base-dll DIR --base-book BOOK --fen FEN
                  --alt MOVE --pinned MOVE --deck "| ballot moves | A=W" [--blocks 3] [--per-block 4]
                  [--fresh-engines] [--eval-only]
  wp12_confirm.py stage3 --tag T --cand-dll DIR --cand-book BOOK --fen FEN --alt MOVE
                  --context NAME="| ballot moves | A=W" ... --nominating NAME [--fresh-engines] [--eval-only]
  env MATCH, CBHOST: runner and host (default bin/match-ddd2, bin/ddd2/cbhost.exe)

Stage 2 (confirmation against Cake 1.89f): BLOCKS separately started blocks; in each, PER-BLOCK games with the
candidate DLL and PER-BLOCK games with the control DLL (the unchanged baseline) are played at the same time, two
matches of -c PER-BLOCK under one hold of the timing lock (8 games at once, the load of one -c 8 match), from the
same deck line. Book mode 3 read back for all three engines before every block; Windows free commit >= 14 GB. A
block counts only if both matches exited 0, played all their games and every game ended on the board; otherwise its
directories are archived (`.failed-<time>`, nothing deleted) and the script stops (exit 5); a new start plays it again.
Verdict by wp12_gates.eval_stage2 (12 + 12 games, >= 3 blocks, >= 4 wins in games that consulted the key, no loss,
>= 3 more such wins than control wins).

Stage 3 (other opponents, other ways in): 12 games per (opponent, context) of wp12_gates.stage3_plan, one match per
opponent (bl_run_w.sh: its options, pre-flight read-back and completion rule), deck = 12 fixed-colour lines per
context. Verdict by wp12_gates.eval_stage3.

Output: ~/regicide-jobs/booklab/wp12/confirm/T/{stage2,stage3}.md and .tsv (every game: arm/opponent, block/context,
round, result, reached, consulted, key ply, termination), manifest.json (hashes of DLLs, books, runner, host).
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
import wp12_gates as G  # noqa: E402
from theory import ballot_of  # noqa: E402

HOME = os.path.expanduser("~")
L = f"{HOME}/regicide-jobs/booklab"
J = "/mnt/c/regicide-match/jobs/booklab"
TOOLS = os.path.dirname(os.path.abspath(__file__))
MATCH = os.environ.get("MATCH", f"{L}/bin/match-ddd2")
CB = os.environ.get("CBHOST", f"{L}/bin/ddd2/cbhost.exe")
E = r"C:\regicide-match\engines"
EW = "/mnt/c/regicide-match/engines"
DB8 = r"C:\regicide-match\db\kingsrow_wld8"
RESULTS = f"{HOME}/regicide-match/results"
LOCK = f"{HOME}/regicide-jobs/locks/timing.lock"
PINWALK = f"{HOME}/regicide-booklab/build-linux/booklab_pinwalk"
EGDB = f"{HOME}/regicide-data/egdb"
PROBE = f"{TOOLS}/book_probe.py"
PS = "/mnt/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe"
CAKE_OPTS = [f"set dbpath {DB8}", "set enable_wld 1", "set dbmbytes 512", "set max_dbpieces 8", "set hashsize 128"]
REG_OPTS = ["set hashsize 128", "set threads 1"]


def st(msg):
    with open(f"{L}/status", "a") as f:
        f.write(f"{msg} {time.strftime('%Y-%m-%dT%H:%M:%S%z')}\n")


def sha(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def readback(dll, opts):
    args = ["python3", PROBE, "--cbhost", CB, "--dll", dll, "--book", "3", "--readback-only"]
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
    c, t = commit_free(), 0
    text = f"commit free {c} GB"
    while c < minimum and t < 600:
        subprocess.run(["/mnt/c/Windows/System32/wsl.exe", "-u", "root", "--", "sh", "-c",
                        "sync; echo 1 > /proc/sys/vm/drop_caches"], capture_output=True)
        time.sleep(30)
        t += 30
        c = commit_free()
        text += f"; page cache dropped, {c} GB after {t} s"
    return text


def regdll(d):
    return f"{E}\\regicide\\{d}\\regicide_cb.dll"


def split_line(line):
    """'| moves | A=W' -> ('moves', 'W')"""
    moves, colour = line.strip().strip("|").rsplit("|", 1)
    return " ".join(moves.split()), colour.strip()[-1]


def pair_filter(recs, colour):
    """Match order (--pairs): keep the games of the candidate's colour; a pair whose other game did not end on the
    board is not counted either (the kept game of that match is marked off the board)."""
    off_runs = {r["run"] for r in recs if r["off_board"]}
    out = [r for r in recs if r["colour"] == colour]
    for r in out:
        if r["run"] in off_runs and not r["off_board"]:
            r["off_board"], r["term"] = True, r["term"] + "; a game of the same match off the board"
    return out


def match_cmd(dll, deck, out, conc, fresh, pairs=False):
    opts = []
    for o in REG_OPTS + ["set book 3"]:
        opts += ["--option-a", o]
    for o in CAKE_OPTS + ["set book 3"]:
        opts += ["--option-b", o]
    return [MATCH, "--allow-rejected-options"] + (["--fresh-engines"] if fresh else []) + (
        ["--pairs-together"] if pairs else []) + [
        "-a", f'exec:{CB} "{dll}"', "-b", f'exec:{CB} "{E}\\cake\\cake_189f.dll"'] + opts + [
        "--openings", deck, "--movetime", "0.5", "-c", str(conc), "--pdn-comments", "-o", out]


def complete(out, n):
    """the match in OUT exited 0, played n games, and every game ended on the board"""
    if not (os.path.exists(f"{out}/summary.txt") and os.path.exists(f"{out}/exit_status")):
        return False
    if open(f"{out}/exit_status").read().strip() != "0":
        return False
    if not any(l.startswith(f"Games: {n} of ") for l in open(f"{out}/summary.txt")):
        return False
    gs = G.load_games(f"{out}/games.pdn")
    return len(gs) == n and all(G.on_board(g["term"]) and g["res"] for g in gs)


def write_report(path, title, recs, verdict, why, counts, cols):
    with open(path + ".tsv", "w") as fo:
        fo.write("\t".join(cols) + "\n")
        for r in recs:
            fo.write("\t".join(str(r.get(k, "")) for k in cols) + "\n")
    lines = [f"# {title}", "", f"**Verdict: {verdict}**" + (" — " + "; ".join(why) if why else ""), "",
             "Counts: " + json.dumps(counts, sort_keys=True, default=dict), "",
             "| " + " | ".join(cols) + " |", "|" + "---|" * len(cols)]
    lines += ["| " + " | ".join(str(r.get(k, "")) for k in cols) + " |" for r in recs]
    open(path + ".md", "w").write("\n".join(lines) + "\n")
    print("\n".join(lines[:5]))


def stage2(a, outdir):
    deck = f"{outdir}/stage2_deck.txt"
    moves, colour = split_line(a.deck)
    # --pairs (match order): PER-BLOCK full pairs per arm and block, our Black game first; only the candidate's
    # colour is judged; n = games per match
    n = 2 * a.per_block if a.pairs else a.per_block
    with open(deck, "w") as fo:
        fo.write(((moves if a.pairs else a.deck.strip()) + "\n") * a.per_block)
    arms = dict(cand=(a.cand_dll, a.cand_book, a.alt), ctrl=(a.base_dll, a.base_book, a.pinned))
    dirs = {(b, arm): f"{RESULTS}/bl-w12s2-{a.tag}-b{b}-{arm}" for b in range(1, a.blocks + 1) for arm in arms}
    if not a.eval_only:
        for b in range(1, a.blocks + 1):
            if all(complete(dirs[(b, arm)], n) for arm in arms):
                continue
            st(f"BL wait-lock w12s2-{a.tag}-b{b} (candidate and control, {a.per_block} games each vs cake)")
            with open(LOCK, "w") as lk:
                fcntl.flock(lk, fcntl.LOCK_EX)
                # read-backs under the lock (review 2026-11-11, as bl_run_w.sh): loading a DLL maps its database
                oks = [readback(regdll(a.cand_dll), REG_OPTS), readback(regdll(a.base_dll), REG_OPTS),
                       readback(f"{E}\\cake\\cake_189f.dll", CAKE_OPTS)]
                if not all(ok for ok, _ in oks):
                    fcntl.flock(lk, fcntl.LOCK_UN)
                    st(f"W12S2 {a.tag} refused: book read-back: {' / '.join(t for _, t in oks)}")
                    sys.exit(3)
                mem = ensure_memory()
                st(f"BL start w12s2-{a.tag}-b{b}; {mem}; runner {MATCH}; host {CB}; fresh {int(a.fresh_engines)}")
                procs = []
                for arm, (dll, _, _) in arms.items():
                    out = dirs[(b, arm)]
                    if os.path.exists(out):
                        os.rename(out, f"{out}.partial-{time.strftime('%Y%m%dT%H%M%S')}")
                    logf = open(f"{out}.log", "w")
                    procs.append((subprocess.Popen(match_cmd(regdll(dll), deck, out, a.per_block, a.fresh_engines,
                                                             a.pairs),
                                                   stdout=logf, stderr=subprocess.STDOUT, stdin=subprocess.DEVNULL),
                                  out, logf))
                for p, out, logf in procs:
                    rc = p.wait()
                    logf.close()
                    os.makedirs(out, exist_ok=True)
                    open(f"{out}/exit_status", "w").write(f"{rc}\n")
                fcntl.flock(lk, fcntl.LOCK_UN)
            bad = [out for _, out, _ in procs if not complete(out, n)]
            if bad:
                for out in bad:
                    os.rename(out, f"{out}.failed-{time.strftime('%Y%m%dT%H%M%S')}")
                st(f"BL CRASHES w12s2-{a.tag}-b{b}: {len(bad)} match(es) incomplete or off the board; archived")
                sys.exit(5)
            st(f"BL end w12s2-{a.tag}-b{b}")
    recs = []
    for (b, arm), out in sorted(dirs.items()):
        if not complete(out, n):
            continue
        _, book, exp = arms[arm]
        rr = G.analyse_run(f"{out}/games.pdn", PINWALK, book, EGDB, a.fen, exp, arm=arm, block=f"b{b}",
                           run=os.path.basename(out))
        recs += pair_filter(rr, colour) if a.pairs else rr
    v, why, c = G.eval_stage2(recs)
    write_report(f"{outdir}/stage2", f"WP12 stage 2, {a.tag}: {a.alt} instead of {a.pinned} at {a.fen}", recs, v, why,
                 c, ["arm", "block", "run", "round", "result", "reached", "consulted", "key_ply", "plies", "term"])
    st(f"W12S2 {a.tag} verdict {v}: cand {c['cand_W']}/{c['cand_D']}/{c['cand_L']}, exposed wins {c['exposed_wins']}, "
       f"control wins {c['ctrl_W']}, blocks {c['blocks']}")
    return v


def stage3(a, outdir):
    ctx = dict(x.split("=", 1) for x in a.context)
    if a.nominating not in ctx:
        sys.exit(f"nominating context {a.nominating} is not among --context")
    plan = G.stage3_plan(list(ctx), a.nominating)
    by_line = {split_line(line): name for name, line in ctx.items()}
    recs = []
    for opp in ("cake", "kingsrow", "guinn"):
        names = [c for o, c in plan if o == opp]
        if not names:
            continue
        deck = f"{outdir}/stage3_{opp}.txt"
        with open(deck, "w") as fo:
            for n in names:
                fo.write(((split_line(ctx[n])[0] if a.pairs else ctx[n].strip()) + "\n") * G.S3_GAMES)
        tag = f"w12s3-{a.tag}-{opp}"
        if not a.eval_only:
            env = dict(os.environ, MATCH=MATCH, CBHOST=CB,
                       EXTRA_ARGS="--allow-rejected-options" + (" --fresh-engines" if a.fresh_engines else "")
                       + (" --pairs-together" if a.pairs else ""))
            rc = subprocess.run(["bash", f"{J}/bl_run_w.sh", f"regicide:{a.cand_dll}", opp, tag, deck, "0.5", "8"],
                                env=env, stdin=subprocess.DEVNULL).returncode
            if rc != 0:
                st(f"W12S3 {a.tag} stopped: {tag} rc {rc}")
                sys.exit(rc)
        pdn = f"{RESULTS}/bl-{tag}/games.pdn"
        if not os.path.exists(pdn):
            continue
        rr = G.analyse_run(pdn, PINWALK, a.cand_book, EGDB, a.fen, a.alt, run=f"bl-{tag}")
        off = any(r["off_board"] for r in rr)  # a pair's other game off the board: the run is not counted
        for r, g in zip(rr, G.load_games(pdn)):
            r["context"] = by_line.get((" ".join(ballot_of(g["tags"]["Opening"])[1].split()), g["col"]), "?")
            if a.pairs and r["context"] == "?":
                continue  # the pair's other-colour game (not a planned context)
            if off and not r["off_board"]:
                r["off_board"], r["term"] = True, r["term"] + "; a game of the same match off the board"
            recs.append(r)
    v, why, per = G.eval_stage3(recs, plan)
    write_report(f"{outdir}/stage3", f"WP12 stage 3, {a.tag}: {a.alt} at {a.fen}", recs, v, why,
                 {o: dict(c) for o, c in per.items()},
                 ["opp", "context", "run", "round", "result", "reached", "consulted", "key_ply", "plies", "term"])
    st(f"W12S3 {a.tag} verdict {v}: " + "; ".join(f"{o} {dict(c)}" for o, c in sorted(per.items())))
    return v


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("stage", choices=["stage2", "stage3"])
    ap.add_argument("--tag", required=True)
    ap.add_argument("--cand-dll", required=True)
    ap.add_argument("--cand-book", required=True)
    ap.add_argument("--base-dll")
    ap.add_argument("--base-book")
    ap.add_argument("--fen", required=True)
    ap.add_argument("--alt", required=True)
    ap.add_argument("--pinned")
    ap.add_argument("--deck")
    ap.add_argument("--blocks", type=int, default=3)
    ap.add_argument("--per-block", type=int, default=4)
    ap.add_argument("--context", nargs="*", default=[])
    ap.add_argument("--nominating")
    ap.add_argument("--fresh-engines", action="store_true")
    ap.add_argument("--pairs", action="store_true", help="match order: full pairs with --pairs-together, our Black "
                    "game first; only the candidate's (context's) colour is judged")
    ap.add_argument("--eval-only", action="store_true")
    a = ap.parse_args()
    outdir = f"{L}/wp12/confirm/{a.tag}"
    os.makedirs(outdir, exist_ok=True)
    man = dict(stage=a.stage, cand_dll=a.cand_dll, cand_dll_sha256=sha(f"{EW}/regicide/{a.cand_dll}/regicide_cb.dll"),
               cand_book_sha256=sha(a.cand_book), runner_sha256=sha(MATCH), host_sha256=sha(CB), fen=a.fen, alt=a.alt,
               fresh_engines=a.fresh_engines, pairs=a.pairs, cake_sha256=sha(f"{EW}/cake/cake_189f.dll"))
    if a.stage == "stage2":
        if not (a.base_dll and a.base_book and a.pinned and a.deck):
            sys.exit("stage2 needs --base-dll, --base-book, --pinned and --deck")
        if a.blocks * a.per_block != G.S2_GAMES:
            sys.exit(f"blocks x per-block must be {G.S2_GAMES}")
        man.update(base_dll=a.base_dll, base_dll_sha256=sha(f"{EW}/regicide/{a.base_dll}/regicide_cb.dll"),
                   base_book_sha256=sha(a.base_book), pinned=a.pinned, deck=a.deck)
    else:
        if not (a.context and a.nominating):
            sys.exit("stage3 needs --context and --nominating")
        man.update(contexts=a.context, nominating=a.nominating)
    mpath = f"{outdir}/manifest_{a.stage}.json"
    if os.path.exists(mpath) and json.load(open(mpath)) != man:
        sys.exit(f"{mpath} differs from this call (other DLL, book, runner or settings); use a new --tag")
    json.dump(man, open(mpath, "w"), indent=1, sort_keys=True)
    v = stage2(a, outdir) if a.stage == "stage2" else stage3(a, outdir)
    print(f"verdict {v}")


if __name__ == "__main__":
    main()
