#!/usr/bin/env bash
# Audit item 5: data a reader needs to re-derive the tables, into experiments/truth-data/data (compressed).
source /mnt/c/regicide-match/jobs/truth/common.sh
T=~/regicide-truth/experiments/truth-data; D=$T/data; W=~/regicide-truth-work; O=~/regicide-data/truth
PY=~/regicide-nnue/.venv/bin/python
mkdir -p $D $T/matches
# 1. pilot per-record output (columns: file, record index, pieces, stm, stored score, stored result (black), unused,
#    search scores at 10k/50k/200k nodes); pilot2 = the same records with the label.hpp rule
gzip -9c $J/pilot.csv > $D/pilot_2026-11-09.csv.gz
[ -s $J/pilot2.csv ] && grep -q "pilot2 end rc=0" $J/status && gzip -9c $J/pilot2.csv > $D/pilot2_label_rule.csv.gz
# 2. held-out proven set (DataRecords) and the per-record evals behind the database-truth table
for f in $O/tails/proven_*_9_12.bin; do gzip -9c $f > $D/heldout_$(basename $f).gz; done
SETS=""; for f in $O/tails/proven_*_9_12.bin; do SETS="$SETS --set $f"; done
NETS="--net n4c=$HOME/regicide-nnue4-work/n4c.nnue --net n6c=$HOME/regicide-nnue6-work/n6c.nnue --net n7b=$HOME/regicide-nnue7-work/n7b.nnue"
for n in n8a n8ak n8b n8bk n8bn n8bk130 n8bl6 n8bl7 n8p n8p2 n8c n8q n8bn2; do [ -s $W/$n.ok ] && NETS="$NETS --net $n=$W/$n.nnue"; done
(cd ~/regicide-truth/experiments/truth-data/scripts && OMP_NUM_THREADS=1 nice $PY db_truth.py $SETS $NETS \
  --general ~/regicide-nnue5-work/val2/nnue_valid_1M.bin --general-n 100000 --dump $D/db_truth_per_record.csv.gz) > $J/db_truth_full.md
# 3. the ponder agent's n6c reference runs against Cake (config A), copied with their origin
for r in pt-s2-a1-cake pt-s2-a2-cake; do
  m=$T/matches/ref-ponder-$r; mkdir -p $m; cp ~/regicide-match/results/$r/summary.txt ~/regicide-match/results/$r/results.csv $m/
  echo "Copied from ~/regicide-match/results/$r: a run by the ponder agent (jobs/ponder/run_p.sh, config A: DLL ponder-df7780a, built-in n6c, threads 1, ponder 0, book 0, acf156 x 2, 0.5 s/move). Used here as n6c's reference vs Cake." > $m/NOTE.txt
done
# 4. E5 production data: census (game boundaries from the GAME_START flags) and seed provenance
~/regicide-truth/build/truth census --egdb ~/regicide-data/egdb $O/gen6a_n50k_egdb.bin $O/gen6b_n50k_egdb.bin > $D/e5_census.txt 2>&1
{ echo "seed file: ~/regicide-data/truth/seeds_real_6_30.txt, sha256 $(sha256sum $O/seeds_real_6_30.txt | cut -c1-64)"
  echo "made by: truth seeds --min 6 --max 30 --opponents cake,kingsrow,kings,guinn --out seeds_real_6_30.txt <the PDNs below>"
  echo "result: 25113 games read, 25113 used (cake 11542, guinn 5770, kings 7801), 227871 distinct positions after plies 6..30"
  echo "PDN files ($(wc -l < $J/pdn_results.txt)):"; cat $J/pdn_results.txt; } > $D/e5_seed_provenance.txt
gzip -9c $O/seeds_real_6_30.txt > $D/seeds_real_6_30.txt.gz
st "evidence collected in $D ($(du -sh $D | cut -f1))"
