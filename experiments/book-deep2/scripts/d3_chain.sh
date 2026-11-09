#!/usr/bin/env bash
# Run inside an attached wsl.exe. book_deep3, after the deep2 tests and the round-7 matches:
#  0. base = book_deep2 if deep2 passed, else book_deep. Pass = targeted deep2 - deep (pooled d2t1-3 vs d1t1-3) 95% CI
#     lower bound > 0, and full deep2 vs Cake not worse than the pooled book_deep on-runs by more than 2 Elo (point
#     estimate), and deep2 vs KingsRow not worse than book_deep vs KingsRow by more than 2 Elo (point estimate)
#  1. book_deep3 = merge_books.py base + book_deep_acf156.bin (origin/t3/book-deep-acf156 @ a1ea0a0, 2,532 positions,
#     30M nodes/move, all 156 ballot roots); deeper entry per position+move wins; avoid entries kept; re-saved through
#     bookgen --resume (default roots: start + all 3-ply positions) --expansions 0
#  2. DLL 6bb8c6c-deep3-egdb8; reference = master DLL 6bb8c6c-egdb8 (book_deep)
#  3. vs Cake, acf156 312 games, interleaved: master, deep3; a second pair if the first is promising (Elo > 0) but not
#     significant (CI lower bound <= 0); then deep3 vs KingsRow once. gen5c alongside.
J=~/regicide-jobs/r4; J3=~/regicide-jobs/deep3; BK=~/regicide-data/book; P=/mnt/c/regicide-match/jobs/tm/pool.py
R=/mnt/c/regicide-match/jobs/deep2/run_ba3.sh; BG=~/regicide-jobs/deep2/bin/bookgen-6bb8c6c
st() { echo "D3 $* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
mkdir -p $J3
until grep -q '^R7 R7_DONE' $J/status; do sleep 30; done
while [ -e $J/matchlock ] || [ -e $J/baselock ]; do sleep 20; done
# 0. base
el() { python3 $P "a=$1" "b=$2" | tail -n 1 | sed -E 's/^a - b: ([-+0-9.]+) Elo \[([-+0-9.]+), .*/\1 \2/'; }
read TD TL <<< "$(el ba-d2t1-cake,ba-d2t2-cake,ba-d2t3-cake ba-d1t1-cake,ba-d1t2-cake,ba-d1t3-cake)"
read FD FL <<< "$(el ba-deep2-full-cake ba-deep-on-cake,ba-deep-on2-cake,ba-deep-on3-cake)"
read KD KL <<< "$(el ba-deep2-full-kingsrow ba-deep-on-kingsrow)"
if awk "BEGIN{exit !($TL > 0 && $FD >= -2 && $KD >= -2)}"; then BASE=book_deep2; else BASE=book_deep; fi
st "deep2 check: targeted deep2-deep $TD (CI low $TL), full Cake vs book_deep on-runs $FD, KingsRow vs book_deep $KD -> base $BASE"
# 1. merge
git -C ~/regicide-nnue fetch -q origin
git -C ~/regicide-nnue show a1ea0a0:experiments/book-deep-acf156/book_deep_acf156.bin > $BK/book_deep_acf156.bin
sha256sum $BK/book_deep_acf156.bin > $J3/acf156_sha.txt
python3 /mnt/c/regicide-match/jobs/deep2/merge_books.py $BK/$BASE.bin $BK/book_deep_acf156.bin $BK/book_deep3_merged.bin > $J3/merge.log 2>&1
st "merge $BASE + book_deep_acf156: $(cat $J3/merge.log)"
$BG --resume $BK/book_deep3_merged.bin --expansions 0 --nodes 30000000 --threads 20 --out $BK/book_deep3.bin > $J3/finalize.log 2>&1
st "book_deep3: rc=$? $(grep -E 'avoid list|resumed|wrote' $J3/finalize.log | tr '\n' ' ')"
# 2. DLL
SHA=6bb8c6c OUT=6bb8c6c-deep3-egdb8 BOOKFILE=$BK/book_deep3.bin JOBS=12 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh > $J3/build_deep3.txt 2>&1
B=~/regicide-match/builds/6bb8c6c-deep3-egdb8
{ echo "-- linux bench with set book off"; printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep -E '^bench'
  echo "-- cbhost"; printf 'cmd get dbpieces\ncmd get book\ncmd set avoidfile builtin\nquit\n' | \
    /mnt/c/regicide-match/bin/b71dfa9/cbhost.exe 'C:\regicide-match\engines\regicide\6bb8c6c-deep3-egdb8\regicide_cb.dll' 2>&1 | tr -d '\r'; } >> $J3/build_deep3.txt
st "DLL built: $(grep -E '^bench|^reply' $J3/build_deep3.txt | tr '\n' ' ')"
# 3. tests
bash $R 6bb8c6c-egdb8 d3ref1 cake 0.5
bash $R 6bb8c6c-deep3-egdb8 d3p1 cake 0.5
read D1 L1 <<< "$(el ba-d3p1-cake ba-d3ref1-cake)"
st "pair 1: deep3 - master $D1 (CI low $L1)"
if awk "BEGIN{exit !($D1 > 0 && $L1 <= 0)}"; then
  bash $R 6bb8c6c-egdb8 d3ref2 cake 0.5
  bash $R 6bb8c6c-deep3-egdb8 d3p2 cake 0.5
  st "pooled 2 pairs: $(python3 $P 'deep3=ba-d3p1-cake,ba-d3p2-cake' 'master=ba-d3ref1-cake,ba-d3ref2-cake' | tail -n 1)"
fi
bash $R 6bb8c6c-deep3-egdb8 d3p1 kingsrow 0.5
st "D3_ALL_DONE"
