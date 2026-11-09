#!/usr/bin/env bash
# Run inside an attached wsl.exe. The targeted test showed book2 fixes White Doctor (B), Wilderness II (B) and partly #7 (B)
# but turns Double Cross (W) from 6/30 into 30/30 losses (book2 switches Regicide's 2nd move 18-14 -> 26-23). So:
#  1. stop d2_extend.sh (its unfinished run is set aside)
#  2. book2b = book2 without the Double Cross root (bookgen --resume book2.bin --roots roots2b.txt --expansions 0)
#  3. book_deep2b = merge(book_deep, book2b) re-saved with --roots roots.txt (46) --avoid avoid2.bin (same 6 entries)
#  4. DLL 6bb8c6c-deep2b-egdb8, then targeted 100-game runs, deep2b and book_deep (reference) alternating, until 21:12.
J=~/regicide-jobs/r4; J2=~/regicide-jobs/deep2; JD=~/regicide-jobs/deep; BK=~/regicide-data/book
BG=$J2/bin/bookgen-6bb8c6c; R=/mnt/c/regicide-match/jobs/deep2/run_ba3.sh
st() { echo "D2 $* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
for p in $(pgrep -f 'bash /mnt/c/regicide-match/jobs/deep2/d2_extend.sh'); do kill $p; done
sleep 3
for p in $(pgrep -f 'match-747ab51 .*results/ba-d[12]t'); do kill $p; done
sleep 5
for d in ~/regicide-match/results/ba-d[12]t*-cake; do
  grep -q '^Elo (pent' $d/summary.txt 2>/dev/null || { mv $d $J2/interrupted-$(basename $d); st "interrupted run set aside: $(basename $d)"; }
done
st "d2_extend stopped; building book_deep2b (book2 without the Double Cross root)"
DC='W:W21,22,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,10,11,12,23'
grep -v -x "$DC" $J2/roots2.txt > $J2/roots2b.txt
$BG --resume $BK/book2.bin --roots $J2/roots2b.txt --expansions 0 --nodes 150000000 --threads 20 --out $BK/book2b.bin > $J2/book2b.log 2>&1
python3 /mnt/c/regicide-match/jobs/deep2/merge_books.py $BK/book_deep.bin $BK/book2b.bin $BK/book_deep2b_merged.bin > $J2/merge2b.log 2>&1
$BG --resume $BK/book_deep2b_merged.bin --roots $JD/roots.txt --avoid $J2/avoid2.bin --expansions 0 --nodes 150000000 --threads 20 \
    --out $BK/book_deep2b.bin > $J2/finalize2b.log 2>&1
st "book_deep2b: roots2b $(wc -l < $J2/roots2b.txt); book2b $(grep wrote $J2/book2b.log | cut -c1-80); merge $(cat $J2/merge2b.log); $(grep -E 'avoid list|wrote' $J2/finalize2b.log | tr '\n' ' ')"
SHA=6bb8c6c OUT=6bb8c6c-deep2b-egdb8 BOOKFILE=$BK/book_deep2b.bin JOBS=16 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh > $J2/build_deep2b.txt 2>&1
B=~/regicide-match/builds/6bb8c6c-deep2b-egdb8
{ echo "-- linux bench with set book off"; printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep -E '^bench'
  echo "-- cbhost"; printf 'cmd get dbpieces\ncmd get book\ncmd set avoidfile builtin\nquit\n' | \
    /mnt/c/regicide-match/bin/b71dfa9/cbhost.exe 'C:\regicide-match\engines\regicide\6bb8c6c-deep2b-egdb8\regicide_cb.dll' 2>&1 | tr -d '\r'; } >> $J2/build_deep2b.txt
st "DLL deep2b built: $(grep -E '^bench|^reply' $J2/build_deep2b.txt | tr '\n' ' ')"
export OPENINGS=$J2/target_openings.txt
DEADLINE=$(date -d 'today 21:12' +%s)
i=1; RUN=500
while [ $(( $(date +%s) + RUN )) -le $DEADLINE ]; do
  t0=$(date +%s); bash $R 6bb8c6c-deep2b-egdb8 d2bt$i cake 0.5; RUN=$(( $(date +%s) - t0 + 20 ))
  [ $(( $(date +%s) + RUN )) -le $DEADLINE ] || break
  j=$(( i + 3 )); while grep -q '^Elo (pent' ~/regicide-match/results/ba-d1t$j-cake/summary.txt 2>/dev/null; do j=$((j+1)); done
  bash $R 6bb8c6c-egdb8 d1t$j cake 0.5
  i=$((i + 1))
done
st "D2B_DONE: $((i)) deep2b runs"
