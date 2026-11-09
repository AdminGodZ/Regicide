#!/usr/bin/env bash
# WP4 build step: master's book (src/weights/book.bin at origin/master = book_deep2b) + decisions.txt -> book binary,
# then (optionally) a CheckerBoard DLL with it.
#   make_book.sh DECISIONS OUTBOOK [DLLNAME]
set -euo pipefail
D=$1; OUT=$2; NAME=${3:-}
W=~/regicide-booklab
BASE=~/regicide-jobs/booklab/book_master.bin
$W/build-linux/booklab_apply --book $BASE --decisions $D --out $OUT --report ${OUT%.bin}.report.txt | tail -1
[ -n "$NAME" ] && bash /mnt/c/regicide-match/jobs/booklab/bl_build_dll.sh $NAME $OUT 6 | tail -6
exit 0
