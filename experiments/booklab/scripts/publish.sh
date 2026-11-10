#!/usr/bin/env bash
# Sync tools, scripts, decisions, book and tables into the booklab worktree, commit with MSGFILE and push t3/booklab.
#   publish.sh MSGFILE [BOOK]   (BOOK: the book binary to ship as experiments/booklab/book_booklab.bin)
set -e
M=$1; BOOK=${2:-}
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab; W=~/regicide-booklab; E=$W/experiments/booklab
bash $J/install_src.sh > /dev/null
mkdir -p $E/scripts $E/census $E/tests $E/repairs
cp $J/src/experiments/booklab/README.md $J/src/experiments/booklab/RESULTS.md $E/
for f in census_report.sh deep1.sh wait_next.sh v_run.sh bl_run.sh bl_target.sh bl_build_dll.sh census_wp1.sh make_book.sh assemble.sh publish.sh wait.sh survey.sh theory_build.sh patch_bookgen.py install_src.sh commit.sh acf156_h1.txt acf156_h2.txt; do
  [ -f $J/$f ] && cp $J/$f $E/scripts/
done
for f in $J/t_*.sh $J/v_*.sh; do [ -f "$f" ] && cp "$f" $E/scripts/; done
[ -f $L/decisions.txt ] && cp $L/decisions.txt $E/decisions.txt
[ -d $L/census_out ] && cp $L/census_out/* $E/census/
for d in $L/repairs_*; do [ -d "$d" ] && { mkdir -p $E/repairs/$(basename $d); cp $d/*.txt $E/repairs/$(basename $d)/; }; done
for d in ~/regicide-match/results/bl-*; do
  n=$(basename $d); [ -f $d/summary.txt ] || continue
  case $n in bl-smoke-*) continue ;; esac
  mkdir -p $E/tests/${n#bl-}; cp $d/summary.txt $d/results.csv $E/tests/${n#bl-}/
done
grep -v "^THEORY" $L/status > $E/timeline.txt || true
[ -n "$BOOK" ] && cp $BOOK $E/book_booklab.bin
cd $W
mkdir -p $E/wp9
for d in $L/wp9/*/; do [ -f $d/positions.tsv ] && { mkdir -p $E/wp9/$(basename $d); cp $d/*.txt $d/positions.tsv $E/wp9/$(basename $d)/; }; done
cp $L/wp9/report-*.md $E/wp9/ 2>/dev/null || true
cp $J/wp9_run.sh $J/patch_match_colour.py $J/requeue1.sh $E/scripts/ 2>/dev/null || true
git add -A tools/booklab tools/match experiments/booklab CMakeLists.txt tools/bookgen.cpp
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F $M || echo "(nothing to commit)"
git push -q -u origin t3/booklab 2>&1 | tail -2
git log --oneline -1
