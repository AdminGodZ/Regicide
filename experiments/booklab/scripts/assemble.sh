#!/usr/bin/env bash
# assemble.sh OUT.txt REPAIRDIR... : concatenates repair outputs into a decisions file, one section per repair (its
# '#' header lines kept as comments), first decision per position wins (later duplicates dropped, logged).
OUT=$1; shift
{
  echo "# booklab decisions: FEN | move | force|avoid | evidence"
  echo "# Built by assemble.sh from repair outputs (theory.py repair: the opponent's own non-losing defence from the first"
  echo "# position where Regicide's move left it). First decision per position wins. $(date -Is)"
  for d in "$@"; do
    for f in $d/*.txt; do
      echo; echo "# === $(basename $d)/$(basename $f .txt)"
      cat $f
    done
  done
} | awk -F' [|] ' '/^#/ || NF < 3 {print; next} { if (seen[$1]++) { dup++; next } print } END { print "# " dup+0 " duplicate positions dropped" > "/dev/stderr" }' > $OUT
grep -vc "^#\|^$" $OUT
