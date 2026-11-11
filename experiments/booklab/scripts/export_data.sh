#!/usr/bin/env bash
# Audit point A2: export the data behind the numbers in RESULTS.md into experiments/booklab/data (compressed), plus a
# manifest (sha256, bytes, games, location) of every raw games.pdn the numbers come from.
set -e
W=~/regicide-booklab; L=~/regicide-jobs/booklab; R=~/regicide-match/results; D=$W/experiments/booklab/data
J=/mnt/c/regicide-match/jobs/booklab
mkdir -p $D
bash $J/install_src.sh > /dev/null
cp $J/src/tools/booklab/*.hpp $W/tools/booklab/
bash $J/build_tree.sh 4 > /dev/null
cp $W/build-linux/booklab_dbscan $L/bin/blk-dbscan
# 1. step-4 and final-validation scan rows with per-ply database trajectories (quiet, no dedup)
for P in s4 f1; do
  O=$L/${P}_quiet; rm -rf $O; mkdir -p $O/runs
  for d in $R/bl-$P-*; do ln -s $d $O/runs/$(basename $d); done
  python3 $W/tools/booklab/dbscan.py --results $O/runs --scan $L/bin/blk-dbscan --egdb ~/regicide-data/egdb --out $O \
    --jobs 4 --exclude "" --no-dedup --quiet > $O/dbscan.log 2>&1
  gzip -9 -c $O/dbscan_games.tsv > $D/${P}_dbscan_games.tsv.gz
done
python3 $W/tools/booklab/entered_losses.py --scan-tsv $L/s4_quiet/dbscan_games.tsv --results $R \
  --replay $W/build-linux/booklab_replay --out $L/s4_quiet/entered_losses.md --list $L/s4_quiet/entered_losses.txt > /dev/null
cp $L/s4_quiet/entered_losses.md $W/experiments/booklab/wp9/entered_losses.md
# 2. census per-game rows and determinism rows
bash $J/census_report.sh > /dev/null
cp $L/census_out/* $W/experiments/booklab/census/
gzip -9 -c $L/census_out/games.csv > $D/census_games.csv.gz
cp $L/census_out/determinism.csv $D/census_determinism.csv
# 3. r8 vs r10: every compared position with both books' moves
python3 $W/tools/booklab/book_equiv.py --runs $R/bl-wp9conv1-* $R/bl-wp9conv3-* $R/bl-s4-* $R/bl-t7-r7c-cake \
  $R/bl-t8-r8c-kingsrow $R/bl-wp9final1-* $R/bl-f1-* --removed $W/experiments/booklab/removed_decisions.txt \
  --replay $W/build-linux/booklab_replay --cmp $W/build-linux/booklab_bookcmp --book-a $L/book_r8.bin \
  --book-b $L/book_r10.bin --out $L/book_equiv_r10.md --dump $L/book_equiv_r10.tsv > /dev/null
cp $L/book_equiv_r10.md $W/experiments/booklab/
gzip -9 -c $L/book_equiv_r10.tsv > $D/book_equiv_r10_positions.tsv.gz
# 4. nodes per second in won 7-8-piece roots (re-measured; the original figures were read from the terminal)
{
  echo "# nps measurement, $(date -Is), engine bl-conv3 tree (2156359), CLI 'go nodes 3000000' with the 2-8-piece db"
  echo "# load at measurement: $(uptime)"
  RG=~/regicide-match/builds/bl-conv3/build-linux/regicide
  for f in "B:WK1,5,K7,20,22,32:B8,19" "W:W5,10,14,20,24:B26,K27,28" "B:W8,K16,21,23:B5,K9,K13,K25" "W:W21-32:B1-12"; do
    echo "== $f"
    printf "set egdb $HOME/regicide-data/egdb\nposition fen $f\ngo nodes 3000000\nquit\n" | $RG 2>&1 | grep "^info" | tail -1
  done
} > $D/nps_won_roots.log 2>&1
# 5. manifest of the raw game files
{
  echo -e "run\tgames\tbytes\tsha256\tlocation"
  for d in $R/bl-* $R/ba-deep2b-full-*; do
    f=$d/games.pdn; [ -f $f ] || continue
    echo -e "$(basename $d)\t$(grep -c '^\[Event' $f)\t$(stat -c %s $f)\t$(sha256sum $f | cut -c1-64)\tWin11-AdminGod:~/regicide-match/results/$(basename $d)/games.pdn"
  done
} > $D/raw_games_manifest.tsv
ls -la $D
