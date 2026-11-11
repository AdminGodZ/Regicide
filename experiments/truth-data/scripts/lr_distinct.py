import re
import statistics as st
rows = [l.split("|")[1:-1] for l in open("/mnt/c/regicide-match/jobs/truth/lossreport_final.md") if l.startswith("| ") and "---" not in l and not l.startswith("| run") and not l.startswith("| opponent") and len(l.split("|")) == 12]
rows = [[c.strip() for c in r] for r in rows]
print("rows", len(rows))
bal = {(r[2], r[3], r[4]) for r in rows}
print("distinct opponent+ballot+colour", len(bal))
games = {}
for r in rows:
    games.setdefault((r[2], r[3], r[4], r[6], r[7]), []).append(r)
print("distinct (opp, ballot, colour, turn pieces, deep before->after)", len(games))
tp = [int(k[3]) for k in games]
pl = [int(re.search(r"\((\d+)\)", g[0][5]).group(1)) for g in games.values()]
bins = [sum(1 for t in tp if lo <= t <= hi) for lo, hi in ((20, 24), (17, 19), (13, 16), (9, 12), (0, 8))]
print("turn bins over distinct", bins, "proven-lost median pieces", st.median(pl))
big = [k for k in games if (lambda a, b: b - a)(*map(int, re.findall(r"-?\d+", k[4]))) <= -100]
print("distinct with drop >= 100:", len(big))
