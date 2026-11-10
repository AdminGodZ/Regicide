#!/usr/bin/env bash
# Exhaustive reachability search with a large node limit for the decisions reach_check left "unknown" (and the 3
# proved unreachable, as a cross-check), against the r8 book. 6 parallel chunks (registered name blk-reach).
#   deep_reach.sh LIMIT
L=~/regicide-jobs/booklab; W=~/regicide-booklab; O=$L/deep_reach; LIMIT=${1:-20000000}
mkdir -p $O; cp $W/build-linux/booklab_reach $L/bin/blk-reach
python3 - $O <<'PY'
import os, re, sys
sys.path.insert(0, os.path.expanduser("~/regicide-booklab/tools/booklab"))
import theory
theory.acf_name("")
deck = {int(re.match(r"ACF #(\d+)", n).group(1)): mv for mv, n in theory._ACF.items()}
L = os.path.expanduser("~/regicide-jobs/booklab")
dec = {n: l.rstrip("\n") for n, l in enumerate(open(f"{L}/decisions_r8.txt"), 1)}
q = []
for l in open(f"{L}/reach_r8_v2.txt"):
    m = re.match(r"line (\d+): (\w+)", l)
    if not m:
        continue
    n = int(m.group(1))
    fen, mv, kind, ev = dec[n].split(" | ", 3)
    b = int(re.search(r"ACF #(\d+)", ev).group(1)); p = int(re.search(r"\bply (\d+)", ev).group(1))
    q.append(f"{n}\t{deck[b]}\t{fen}\t{p + 2}")
o = sys.argv[1]
for j in range(6):
    open(f"{o}/q{j}.tsv", "w").write("\n".join(q[j::6]) + "\n")
print(len(q), "queries")
PY
for j in 0 1 2 3 4 5; do nice $L/bin/blk-reach $L/book_r8.bin $LIMIT ~/regicide-data/egdb < $O/q$j.tsv > $O/r$j.tsv 2>&1 & done
wait
echo "DEEPREACH done $(date -Is)" >> $L/status
