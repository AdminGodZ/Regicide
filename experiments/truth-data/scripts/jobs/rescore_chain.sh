#!/usr/bin/env bash
# E3 rescoring chain (detached, resumable: rerun this script and every step continues where it stopped).
#   1. held-out proven set for the database-truth metric: the 2% validation tails of gen5a, gen5c and gen4a,
#      9-12 pieces, searched at 50k nodes with the 8-piece database; proven records kept.
#   2. rescore the E2 (relabelled) files, gen5a and gen4a first (n8c's data), then gen5c and the rest, newest first.
# Threads: RS_THREADS (default 6: 2 more for my GPU training runs keep me within 8). Budgets: RS_NODES (10k-node
# files, default 9-12 pieces 50k, 13-16 20k), RS_NODES50 (gen5 files, default 50k for 9-16), RS_KEEP (default 16:
# records with 17+ pieces keep their stored score).
# The rescore binary is a copy named rescore-tr1 (pausekeeper_truth stops it while r4/matchlock exists).
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth; EG=~/regicide-data/egdb; B=$J/bin; mkdir -p $B $O/tails
TH=${RS_THREADS:-6}; NODES=${RS_NODES:-9:50000,13:20000}; KEEP=${RS_KEEP:-16}
[ -x $B/rescore-tr4 ] || cp ~/regicide-truth/build/rescore $B/rescore-tr4   # tr2: with the incomplete-search guard (2026-11-10)
[ -x $B/truth-tr2 ] || cp ~/regicide-truth/build/truth $B/truth-tr2
st "rescore_chain start pid $$ threads $TH nodes $NODES keep-above $KEEP"
for f in gen5a_n50k gen5c_n50k gen4a_n10k; do
  [ -s $O/tails/proven_${f}_9_12.bin ] && continue
  n=$(( $(stat -c %s $O/${f}_e2.bin) / 16 )); from=$(( n - n * 2 / 100 ))
  # (python: nv = int(n * 0.02), tail starts at n - nv; for these sizes n*2/100 == int(n*0.02))
  $B/truth-tr2 split --in $O/${f}_e2.bin --out $O/tails/tail_${f}_9_12.bin --min 9 --max 12 --from $from >> $J/rescore_chain.log
  $B/rescore-tr4 --egdb $EG --in $O/tails/tail_${f}_9_12.bin --out $O/tails/tail_${f}_9_12_rs.bin --threads $TH --nodes 9:50000 --keep-above 12 >> $J/rescore_chain.log 2>&1 || exit 1
  $B/truth-tr2 split --in $O/tails/tail_${f}_9_12_rs.bin --out $O/tails/proven_${f}_9_12.bin.tmp --min 9 --max 12 --proven >> $J/rescore_chain.log && mv $O/tails/proven_${f}_9_12.bin.tmp $O/tails/proven_${f}_9_12.bin
  st "proven tail set $f: $(( $(stat -c %s $O/tails/proven_${f}_9_12.bin) / 16 )) records"
done
st "PROVEN_SET_DONE"
for f in gen5a_n50k gen4a_n10k gen5c_n50k gen3d_n10k gen3c_n10k gen3b_n10k gen3a_n10k gen2b_n10k gen2a_n10k gen1c_n10k gen1b_n10k gen1_n10k; do
  [ -s $O/${f}_e3.bin ] && continue
  st "rescore $f start"
  nodes=$NODES; [[ $f == *n50k ]] && nodes=${RS_NODES50:-9:50000}  # 50k-node files: never below their original budget
  $B/rescore-tr4 --egdb $EG --in $O/${f}_e2.bin --out $O/${f}_e3.bin.part --threads $TH --nodes $nodes --keep-above $KEEP >> $J/rescore_$f.log 2>&1 || { st "rescore $f stopped rc=$?"; exit 1; }
  mv $O/${f}_e3.bin.part $O/${f}_e3.bin
  st "rescore $f done: $(tail -n 2 $J/rescore_$f.log | head -n 1)"
done
st "RESCORE_ALL_DONE"
