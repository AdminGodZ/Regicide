#!/usr/bin/env bash
# WP4 build step: master's book (src/weights/book.bin at origin/master = book_deep2b) + decisions.txt -> book binary,
# then the reachability check of every decision under the built book (unreachable ones are listed; the build fails
# if any decision is proved unreachable), and optionally a CheckerBoard DLL with it.
#   make_book.sh DECISIONS OUTBOOK [DLLNAME]      (env NOREACH=1 skips the reachability check)
set -euo pipefail
D=$1; OUT=$2; NAME=${3:-}
W=~/regicide-booklab; L=~/regicide-jobs/booklab
BASE=$L/book_master.bin
$W/build-linux/booklab_apply --book $BASE --decisions $D --out $OUT --report ${OUT%.bin}.report.txt | tail -1
if [ -z "${NOREACH:-}" ]; then
  python3 $W/tools/booklab/reach_check.py --decisions $D --book $OUT --reach $W/build-linux/booklab_reach \
    --db $L/theory.sqlite --limit 200000 | tee ${OUT%.bin}.reach.txt | tail -12
fi
[ -n "$NAME" ] && bash /mnt/c/regicide-match/jobs/booklab/bl_build_dll.sh $NAME $OUT 6 | tail -6
exit 0
