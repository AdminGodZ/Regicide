#!/usr/bin/env bash
# After book_n4c finishes: merge the avoid list into it (bookgen --resume book_n4c.bin --avoid avoid_v1.bin
# --expansions 0: avoided moves and the subtrees below them are dropped, the avoid entries are written into the book,
# RGBK v2; bookgen from 65f7143), then build the Test-B DLL: master 769ef0b with that file embedded as src/weights/book.bin (+ 2-8 egdb copy).
J=~/regicide-jobs/r4; JB=~/regicide-jobs/ba; SHA=769ef0b; BG=$JB/bin/bookgen-65f7143
st() { echo "$* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
until grep -q '^BOOK end' $J/status; do sleep 30; done
BK=~/regicide-data/book; A=/mnt/c/regicide-match/avoid/avoid_v1.bin
cp $A $BK/avoid_v1.bin
[ -e $BK/book_n4c_avoid.bin ] && { st "BA book_post refusing: book_n4c_avoid.bin exists"; exit 1; }
nice -n 5 $BG --resume $BK/book_n4c.bin --avoid $BK/avoid_v1.bin --expansions 0 --nodes 1000000 \
  --threads 10 --out $BK/book_n4c_avoid.bin > $JB/book_avoid_merge.log 2>&1
st "BA book+avoid merge rc=$? $(grep -E 'avoid list|resumed|wrote' $JB/book_avoid_merge.log | tr '\n' ' ')"
cp $BK/book_n4c_avoid.bin /mnt/c/regicide-match/avoid/book_n4c_avoid.bin
SHA=$SHA OUT=$SHA-bookavoid-egdb8 BOOKFILE=$BK/book_n4c_avoid.bin JOBS=3 \
  bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh > $JB/build_bookavoid.txt 2>&1
B=~/regicide-match/builds/$SHA-bookavoid-egdb8
echo "-- linux bench with set book off" >> $JB/build_bookavoid.txt
printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep -E '^bench|book' >> $JB/build_bookavoid.txt
CB=/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe
W="C:\\regicide-match\\engines\\regicide\\$SHA-bookavoid-egdb8\\regicide_cb.dll"
echo "-- cbhost: dbpieces, book mode, avoid" >> $JB/build_bookavoid.txt
printf 'cmd get dbpieces\ncmd get book\ncmd get avoidfile\ncmd get avoid\nquit\n' | $CB "$W" 2>&1 | tr -d '\r' >> $JB/build_bookavoid.txt
st "BA_BOOKAVOID_DLL_DONE $(grep -E '^bench' $JB/build_bookavoid.txt | tr '\n' ' ')"
