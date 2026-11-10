#!/usr/bin/env bash
# Cross-check of wp10_report.py exposure against the independent checker (experiments/holdout/classify_exposure.py)
# for the current-engine configurations (E, EB, EBN; not O, whose older book policy the checker does not model).
SRC=~/regicide-booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; E=$SRC/experiments/booklab/wp10
out=$E/exposure_xcheck.tsv; : > $out.tmp
for run in $(cut -f14 $E/games.tsv | sort -u | grep -v "^run$"); do
  case $run in *-O-*) continue ;; *-E-*) pb=old ;; *) pb=new ;; esac
  python3 $SRC/experiments/holdout/classify_exposure.py --helper $L/bin/exposure_helper --old-book $L/book_master.bin \
    --new-book $L/book_r10.bin --our-name 'Regicide 0.2' --played-book $pb $R/$run/games.pdn 2>> $L/xcheck.err \
    | tail -n +2 | sed "s|^$R/||; s|/games.pdn:|\t|" >> $out.tmp
done
mv $out.tmp $out
python3 - $E/games.tsv $out <<'PY'
import csv, sys
mine = {(r["run"], r["idx"]): int(r["exposed"]) for r in csv.DictReader(open(sys.argv[1]), delimiter="\t")}
agree = differ = 0
diffs = []
for l in open(sys.argv[2]):
    f = l.rstrip("\n").split("\t")
    run, idx, cls, first = f[0], f[1], f[2], int(f[3])
    m = mine.get((run, idx))
    if m is None:
        continue
    theirs = first if cls == "exposed" else -1
    if cls not in ("exposed", "unexposed"):
        diffs.append((run, idx, cls, m)); differ += 1; continue
    if theirs == m:
        agree += 1
    else:
        differ += 1; diffs.append((run, idx, f"{cls} {first}", m))
print(f"exposure cross-check: {agree} games agree, {differ} differ")
for d in diffs[:20]:
    print("  differ:", d)
PY
