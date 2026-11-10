#!/usr/bin/env bash
# assemble.sh OUT.txt REPAIRDIR... : concatenates repair outputs into a decisions file, one section per repair file
# (its '#' header lines kept as comments). Positions listed in experiments/booklab/removed_decisions.txt (FEN | move |
# reason) are left out. The first decision per position is kept; a later row for the same position with the SAME move
# is dropped (counted); a later row with a DIFFERENT move or action (force/avoid) is a conflict: both rows are printed and the script fails,
# so a later repair can never be shadowed by an older one unnoticed.
OUT=$1; shift
REMOVED=${REMOVED:-~/regicide-booklab/experiments/booklab/removed_decisions.txt}
{
  echo "# booklab decisions: FEN | move | force|avoid | evidence"
  echo "# Built by assemble.sh from repair outputs (theory.py repair: the opponent's own non-losing defence from the first"
  echo "# position where Regicide's move left it). Positions in removed_decisions.txt are left out. $(date -Is)"
  for d in "$@"; do
    for f in $d/*.txt; do
      echo; echo "# === $(basename $d)/$(basename $f .txt)"
      cat $f
    done
  done
} | awk -F' [|] ' -v removed="$REMOVED" '
  BEGIN { while ((getline l < removed) > 0) { if (l ~ /^#/) continue; split(l, p, " [|] "); if (p[1] != "") rm[p[1]] = 1 } }
  /^#/ || NF < 3 { print; next }
  ($1 in rm) { nrm++; next }
  ($1 in seen) {
    if (seen[$1] != $2 " " $3) { print "CONFLICT: " $1 ": kept \"" seen[$1] "\" (" src[$1] ") vs \"" $2 " " $3 "\" (" $4 ")" > "/dev/stderr"; bad++ }
    else dup++
    next
  }
  { seen[$1] = $2 " " $3; src[$1] = substr($4, 1, 80); print }  # identity: move AND action (force/avoid)
  END { print "# " dup+0 " duplicate rows with the same move dropped, " nrm+0 " removed positions left out, " bad+0 " conflicts" > "/dev/stderr"; if (bad) exit 3 }
' > $OUT
rc=${PIPESTATUS[1]}
[ "$rc" = 0 ] || { echo "assemble.sh: $rc: conflicting decisions, see above; $OUT is incomplete" >&2; exit 1; }
grep -vc "^#\|^$" $OUT
