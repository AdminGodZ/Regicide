"""Item 4 (lead, 2026-11-10): the seven games Regicide lost as Black to KingsRow in book mode 3 (Book Lab,
experiments/booklab/k3 on origin/t3/booklab). For each distinct line: at every position Regicide (Black) had to move,
from the end of the 3-ply ballot to the first position a 20M-node search (n6c, 8-piece database) calls lost:
n6c's and n8q's static eval and 1M-node search score (Black's view), and how often the position itself and its
one-move neighbours (the positions after each legal move) occur in the training data, with their labels.
Searches: this branch's CLI, set evalfile, database, book off, fresh hash, game history kept (position startpos moves).
"""
import gzip
import os
import re
import subprocess
import sys

import numpy as np

sys.path.insert(0, os.path.expanduser("~/regicide-truth/tools/nnue"))
import nnue  # noqa: E402

E = os.path.expanduser("~/regicide-truth/build/regicide")
EGDB = os.path.expanduser("~/regicide-data/egdb")
NETS = {"n6c": os.path.expanduser("~/regicide-nnue6-work/n6c.nnue"), "n8q": os.path.expanduser("~/regicide-truth-work/n8q.nnue")}
GAMES = [("bl-k3-kingsrow-a-h1", "52.1", "#60"), ("bl-k3-kingsrow-b-h1", "52.1", "#60"), ("bl-k3-kingsrow-a-h1", "63.1", "#71"),
         ("bl-k3-kingsrow-b-h1", "63.1", "#71"), ("bl-k3-kingsrow-a-h2", "49.1", "#141"), ("bl-k3-kingsrow-b-h1", "8.1", "#9"),
         ("bl-k3-kingsrow-b-h1", "31.1", "#34")]
OUT = os.path.expanduser("~/regicide-jobs/truth/k3")
os.makedirs(OUT, exist_ok=True)


def git_pdn(run):
    raw = subprocess.run(["git", "-C", os.path.expanduser("~/regicide-truth"), "show",
                          "origin/t3/booklab:experiments/booklab/k3/games/%s.pdn.gz" % run], capture_output=True, check=True).stdout
    return gzip.decompress(raw).decode()


def games(text):
    for chunk in text.split("[Event ")[1:]:
        tags = dict(re.findall(r'^\[?(\w+) "([^"]*)"\]', "[Event " + chunk, re.M))
        body = chunk.split("\n", 1)[1] if "\n" in chunk else ""  # the rest of the Event tag line can contain brackets
        body = re.sub(r"^\[.*\]\s*$", "", body, flags=re.M)
        body = re.sub(r"\{[^}]*\}", " ", body)
        moves = [re.sub(r"^\d+\.+", "", t) for t in body.split()]
        moves = [m for m in moves if m and m not in ("1-0", "0-1", "1/2-1/2", "*")]
        yield tags, moves


def cli(cmds):
    return subprocess.run([E], input="\n".join(cmds) + "\nquit\n", capture_output=True, text=True).stdout


def last_score(block):
    s = re.findall(r"info depth (\d+) .*?score (-?\d+)", block)
    return (int(s[-1][1]), int(s[-1][0])) if s else (None, None)


def fen_bits(fen):
    stm, w, b = fen.split(":")
    bits = {"W": 0, "B": 0}
    kings = 0
    for side, part in (("W", w[1:]), ("B", b[1:])):
        for tok in filter(None, part.split(",")):
            k = tok.startswith("K")
            n = int(tok.lstrip("K"))
            sq = (7 - (n - 1) // 4) * 4 + (n - 1) % 4
            bits[side] |= 1 << sq
            if k:
                kings |= 1 << sq
    return bits["B"], bits["W"], kings, 0 if stm == "B" else 1


def hkey(black, white, kings, stm):
    black, white, kings, stm = (np.asarray(x).astype(np.uint64) for x in (black, white, kings, stm))
    return (black * np.uint64(0x9E3779B97F4A7C15)) ^ ((white << np.uint64(29)) * np.uint64(3)) ^ \
        (kings * np.uint64(0xC2B2AE3D27D4EB4F)) ^ (stm & np.uint64(1))


# --- the games and their lines
lines = {}
for run, rnd, ballot in GAMES:
    g = next((m for t, m in games(git_pdn(run)) if t.get("Round") == rnd), None)
    assert g, (run, rnd)
    lines.setdefault(tuple(g), []).append("%s %s %s" % (ballot, run, rnd))
reps = [(list(k), v) for k, v in lines.items()]
# decision positions (Black to move) after the ballot, with FENs
rows_by_line = []
for moves, names in reps:
    cmds = []
    plies = [k for k in range(3, len(moves)) if k % 2 == 0]
    for k in plies:
        cmds += ["position startpos moves " + " ".join(moves[:k]), "d"]
    fens = re.findall(r"^([BW]:W[^ \n]*:B[^ \n]*)$", cli(cmds), re.M)
    rows_by_line.append({"names": names, "moves": moves, "plies": plies, "fens": fens})
# transpositions between lines (a shared decision position merges them)
merged = []
for r in rows_by_line:
    for m in merged:
        if set(m["fens"]) & set(r["fens"]):
            m["names"] += r["names"]
            m["also"] = m.get("also", []) + [r]
            break
    else:
        merged.append(r)
print("distinct lines: %d" % len(merged))
for m in merged:
    print("  ", m["names"])
# --- 20M walk (n6c) to the first proven loss, then 1M searches and static evals for both nets (cached in walk.pkl)
import pickle  # noqa: E402

CACHE = os.path.join(OUT, "walk.pkl")
cached = pickle.load(open(CACHE, "rb")) if os.path.exists(CACHE) else None
for li, L in enumerate(merged):
    if cached and li < len(cached):
        L.update(cached[li])
        continue
    stop = None
    for c in range(0, len(L["plies"]), 6):
        chunk = L["plies"][c:c + 6]
        cmds = ["set book off", "set egdb " + EGDB, "set evalfile " + NETS["n6c"]]
        for k in chunk:
            cmds += ["newgame", "position startpos moves " + " ".join(L["moves"][:k]), "go nodes 20000000"]
        blocks = cli(cmds).split("bestmove")[:-1]
        for k, b in zip(chunk, blocks):
            s, d = last_score(b)
            L.setdefault("s20", {})[k] = (s, d)
            if s is not None and s <= -15000 and stop is None:
                stop = k
        if stop is not None:
            break
    L["stop"] = stop if stop is not None else L["plies"][-1]
    L["upto"] = [k for k in L["plies"] if k <= L["stop"]]
    for net, path in NETS.items():
        cmds = ["set book off", "set egdb " + EGDB, "set evalfile " + path]
        for k in L["upto"]:
            cmds += ["position startpos moves " + " ".join(L["moves"][:k]), "eval", "newgame",
                     "position startpos moves " + " ".join(L["moves"][:k]), "go nodes 1000000"]
        out = cli(cmds)
        ev = [int(x) for x in re.findall(r"^eval (-?\d+)", out, re.M)]
        sc = [last_score(b)[0] for b in out.split("bestmove")[:-1]]
        L[net] = list(zip(ev, sc))
pickle.dump([{k: L[k] for k in ("s20", "stop", "upto", "n6c", "n8q")} for L in merged], open(CACHE, "wb"))
for L in merged:
    # one-move neighbours: positions after each legal move
    cmds = []
    for k in L["upto"]:
        cmds += ["position startpos moves " + " ".join(L["moves"][:k]), "moves", "eval"]  # eval: a separator line
    out = cli(cmds)
    L["succ"] = {}
    blocks = re.split(r"(?m)^eval -?\d+$", out)
    assert len(blocks) >= len(L["upto"]), (len(blocks), len(L["upto"]))
    for k, blk in zip(L["upto"], blocks):
        mvs = re.findall(r"^\d+: (\S+)", blk, re.M)
        cmds = []
        for mv in mvs:
            cmds += ["position startpos moves " + " ".join(L["moves"][:k] + [mv]), "d"]
        L["succ"][k] = re.findall(r"^([BW]:W[^ \n]*:B[^ \n]*)$", cli(cmds), re.M)
# --- training data: exact matches of the line positions and their neighbours
queries = {}
for li, L in enumerate(merged):
    for idx, k in enumerate(L["upto"]):
        queries.setdefault(L["fens"][L["plies"].index(k)], []).append((li, k, "self"))
        for f in L["succ"][k]:
            queries.setdefault(f, []).append((li, k, "succ"))
qfens = list(queries)
qk = np.array([hkey(*fen_bits(f)) for f in qfens], dtype=np.uint64)
order = np.argsort(qk)
qk_sorted = qk[order]
D = os.path.expanduser("~/regicide-data")
files = [("E2+orig", "%s/truth/%s_e2.bin" % (D, f), "%s/train/%s.bin" % (D, f)) for f in
         ["gen1_n10k", "gen1b_n10k", "gen1c_n10k", "gen2a_n10k", "gen2b_n10k", "gen3a_n10k", "gen3b_n10k", "gen3c_n10k",
          "gen3d_n10k", "gen4a_n10k", "gen5a_n50k", "gen5c_n50k"]] + \
        [("E5", "%s/truth/gen6a_n50k_egdb.bin" % D, None), ("E5", "%s/truth/gen6b_n50k_egdb.bin" % D, None)]
hits = {f: [] for f in qfens}
for kind, f, orig in files:
    m = np.memmap(f, nnue.RECORD, "r")
    n = len(m)
    mo = np.memmap(orig, nnue.RECORD, "r") if orig else None
    for s in range(0, n, 4_000_000):
        chunk = np.asarray(m[s:min(n, s + 4_000_000)])
        h = hkey(chunk["black"], chunk["white"], chunk["kings"], chunk["stm"])
        pos = np.searchsorted(qk_sorted, h)
        pos[pos >= len(qk_sorted)] = 0
        found = np.flatnonzero(qk_sorted[pos] == h)
        for i in found:
            q = qfens[order[pos[i]]]
            r = chunk[i]
            res_orig = int(mo[s + i]["result"]) if mo is not None else None
            hits[q].append((kind, os.path.basename(f), int(r["score"]), int(r["result"]), res_orig,
                            bool(int(r["stm"]) & nnue.PROVEN)))
# --- per-file detail of the exact hits on the line positions
with open(os.path.join(OUT, "k3_hits.txt"), "w") as fh:
    for li, L in enumerate(merged):
        for k in L["upto"]:
            h = hits.get(L["fens"][L["plies"].index(k)], [])
            byf = {}
            for x in h:
                byf.setdefault(x[1], []).append(x)
            for fn, xs in sorted(byf.items()):
                r = [x[3] for x in xs]
                fh.write("line %d ply %d %s: n %d, mean score %+.0f, W/D/L %d/%d/%d, proven %d, distinct scores %d\n" % (
                    li + 1, k, fn, len(xs), np.mean([x[2] for x in xs]), r.count(1), r.count(0), r.count(-1),
                    sum(x[5] for x in xs), len(set(x[2] for x in xs))))
# --- decisions: where the 20M value drops by >= 40 cm over one move pair, which move did the game play, and which
# moves do 20M (n6c) and 1M (n6c, n8q) choose there
with open(os.path.join(OUT, "k3_decisions.txt"), "w") as fh:
    for li, L in enumerate(merged):
        for k in L["upto"]:
            a, b = L["s20"].get(k, (None,))[0], L["s20"].get(k + 2, (None,))[0]
            if a is None or b is None or b - a > -40:
                continue
            row = ["line %d ply %d: 20M %+d -> %+d, game move %s" % (li + 1, k, a, b, L["moves"][k])]
            for net, nodes in (("n6c", 20000000), ("n6c", 1000000), ("n8q", 1000000)):
                out = cli(["set book off", "set egdb " + EGDB, "set evalfile " + NETS[net], "newgame",
                           "position startpos moves " + " ".join(L["moves"][:k]), "go nodes %d" % nodes])
                bm = re.findall(r"bestmove (\S+)", out)
                row.append("%s %dM: %s (%s)" % (net, nodes // 1000000, bm[-1] if bm else "?", last_score(out)[0]))
            fh.write("; ".join(row) + "\n")
            print("; ".join(row), flush=True)
# --- tables
md = []
for li, L in enumerate(merged):
    md.append("\n#### Line %d: %s\n" % (li + 1, "; ".join(L["names"])))
    md.append("Moves: %s\n" % " ".join(L["moves"][:L["stop"] + 2]))
    md.append("| ply | pieces | n6c static / 1M | n8q static / 1M | 20M (n6c) | in training (exact): n, mean score, results W/D/L (E2 / original) | one-move neighbours in training: n, mean score (Black's view) |")
    md.append("|---|---|---|---|---|---|---|")
    for idx, k in enumerate(L["upto"]):
        fen = L["fens"][L["plies"].index(k)]
        pieces = len(re.findall(r"\d+", fen.split(":", 1)[1]))
        h = hits.get(fen, [])
        if h:
            sc = np.mean([x[2] for x in h])
            e2 = [x[3] for x in h]
            og = [x[4] for x in h if x[4] is not None]
            ex = "%d, %+.0f, %d/%d/%d / %d/%d/%d" % (len(h), sc, e2.count(1), e2.count(0), e2.count(-1),
                                                     og.count(1), og.count(0), og.count(-1))
        else:
            ex = "0"
        nb = [x for sf in L["succ"][k] for x in hits.get(sf, [])]
        nbs = "%d, %+.0f" % (len(nb), -np.mean([x[2] for x in nb])) if nb else "0"  # successor: White to move
        s20 = L.get("s20", {}).get(k, (None, None))
        md.append("| %d | %d | %+d / %s | %+d / %s | %s | %s | %s |" % (
            k, pieces, L["n6c"][idx][0], L["n6c"][idx][1], L["n8q"][idx][0], L["n8q"][idx][1],
            "%s (d%s)" % s20 if s20[0] is not None else "-", ex, nbs))
open(os.path.join(OUT, "k3_lines.md"), "w").write("\n".join(md) + "\n")
print("\n".join(md))
