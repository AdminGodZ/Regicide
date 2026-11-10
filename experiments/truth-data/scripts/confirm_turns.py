"""Confirms the estimated turning moves of the five n8q losses (lw_bl-f4-cake-h*.tsv) with a large search (100M nodes,
1 thread, 8-piece database, book off, fresh hash) of the position before and after the move, with n8q's and n6c's
evaluation. Score of the move = -(score of the position after it). No game history (as losswalk)."""
import csv
import os
import re
import subprocess

E = os.path.expanduser("~/regicide-truth/build/regicide")
J = os.path.expanduser("~/regicide-jobs/truth")
NETS = {"n8q": os.path.expanduser("~/regicide-truth-work/n8q.nnue"), "n6c": os.path.expanduser("~/regicide-nnue6-work/n6c.nnue")}
NODES = 100_000_000
rows = []
for f in ("lw_bl-f4-cake-h1.tsv", "lw_bl-f4-cake-h2.tsv"):
    rows += list(csv.DictReader(open(os.path.join(J, f)), delimiter="\t"))
seen = set()
print("| game (Regicide) | turn: pieces | move played | net | search before (Regicide to move) | best move | move played: score after (Regicide's view) |")
print("|---|---|---|---|---|---|---|")
for r in rows:
    key = r["turn_fen_before"]
    if key in seen:
        continue
    seen.add(key)
    ballot = r["opening"].rsplit(": ", 1)[-1]
    for net, path in NETS.items():
        cmds = ["set book off", "set egdb " + os.path.expanduser("~/regicide-data/egdb"), "set evalfile " + path,
                "newgame", "position fen " + r["turn_fen_before"], "go nodes %d" % NODES,
                "newgame", "position fen " + r["turn_fen_after"], "go nodes %d" % NODES, "quit"]
        out = subprocess.run([E], input="\n".join(cmds) + "\n", capture_output=True, text=True).stdout
        blocks = out.split("bestmove")[:-1]
        res = []
        for b in blocks:
            s = re.findall(r"info depth (\d+) .*?score (-?\d+)", b)
            res.append((int(s[-1][0]), int(s[-1][1])) if s else (0, 0))
        best = re.findall(r"bestmove (\S+)", out)[0]
        print("| %s (%s) | %s | %s | %s | %+d (d%d) | %s | %+d (d%d) |" % (
            ballot, r["regicide"], r["turn_pieces"], r["turn_played"], net, res[0][1], res[0][0], best, -res[1][1],
            res[1][0]), flush=True)
