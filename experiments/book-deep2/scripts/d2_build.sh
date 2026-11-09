#!/usr/bin/env bash
# Deep book pass 2: build master 6bb8c6c (built-in n6c + book_deep): Windows DLL 6bb8c6c-egdb8 (the reference "book_deep"
# engine) and its Linux tree (bookgen, lossprobe); checks: embedded book == book_deep.bin, bench 7,712,020 /
# 8,089,571 with set book off, ctest.
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
J2=~/regicide-jobs/deep2; mkdir -p $J2/bin
SHA=6bb8c6c
SHA=$SHA OUT=$SHA-egdb8 JOBS=12 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh
B=~/regicide-match/builds/$SHA-egdb8
cmp $B/src/src/weights/book.bin ~/regicide-data/book/book_deep.bin && echo "embedded book.bin == book_deep.bin"
nice cmake --build $B/build-linux -j 12 > /dev/null
echo "-- linux ctest"; ctest --test-dir $B/build-linux -j 8 2>&1 | tail -n 3
echo "-- linux bench with set book off"; printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep '^bench'
cp $B/build-linux/bookgen $J2/bin/bookgen-$SHA; cp $B/build-linux/lossprobe $J2/bin/lossprobe-$SHA
sha256sum $J2/bin/*
echo "D2_BUILD_DONE $(date -Is)" >> ~/regicide-jobs/r4/status
