#!/usr/bin/env bash
# WP9 step 4 report: W/D/L per DLL and opponent, and for every game that reached a database win (either side)
# whether it was converted (dbscan over the s4 runs only).
#   s4_report.sh PREFIX OUTDIR   (PREFIX e.g. s4 or f1)
P=${1:-s4}; L=~/regicide-jobs/booklab; O=$L/${2:-s4_scan}
T=$O/runs; rm -rf $O; mkdir -p $T
for d in ~/regicide-match/results/bl-$P-*; do ln -s $d $T/$(basename $d); done
python3 /mnt/c/regicide-match/jobs/booklab/src/tools/booklab/dbscan.py --results $T --scan $L/bin/blk-dbscan \
  --egdb ~/regicide-data/egdb --out $O --jobs 4 --exclude "" --no-dedup > $O/dbscan.log 2>&1
python3 - $O <<'PY'
import csv, collections, sys, os
o = sys.argv[1]
rows = list(csv.DictReader(open(os.path.join(o, "dbscan_games.tsv")), delimiter="\t"))
def dll(run):  # bl-s4-conv3-cake-h1 -> conv3 ; bl-f1-cake-a-h1 -> final1
    p = run.split("-")
    return p[2] if p[1] == "s4" else "final1"
def opp(r): return r["wf"] if r["bf"] == "regicide" else r["bf"]
t = collections.defaultdict(lambda: collections.Counter())
for r in rows:
    k = (dll(r["run"]), opp(r))
    col = "B" if r["bf"] == "regicide" else "W"
    res = "D" if r["res"] == "D" else ("W" if r["res"] == col else "L")
    t[k][res] += 1
    if r["winner"] in ("B", "W"):
        side = "regicide" if r["winner"] == col else "opponent"
        t[k][f"{side}_reached"] += 1
        t[k][f"{side}_converted"] += r["res"] == r["winner"]
print("| DLL | Opponent | Games | W / D / L | Regicide reached a db win | converted | Opponent reached a db win | converted |")
print("|---|---|---|---|---|---|---|---|")
for (d, op), c in sorted(t.items()):
    print(f"| {d} | {op} | {c['W'] + c['D'] + c['L']} | {c['W']} / {c['D']} / {c['L']} | {c['regicide_reached']} | "
          f"{c['regicide_converted']} | {c['opponent_reached']} | {c['opponent_converted']} |")
PY
