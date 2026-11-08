#!/usr/bin/env bash
# DLL from master 769ef0b (EGDB cache fix: partial pre-fault smallest first; set dbmbytes raises the budget of the
# loaded database). Two engine dirs: 769ef0b-egdb8 (2-8 copy) and 769ef0b-egdb7 (2-7 copy). Linux ctest + bench.
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
SHA=769ef0b
SHA=$SHA OUT=$SHA-egdb8 JOBS=3 bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh
B=~/regicide-match/builds/$SHA-egdb8
echo "-- linux ctest"; nice ctest --test-dir $B/build-linux -j 3 2>&1 | tail -n 3
D7=/mnt/c/regicide-match/engines/regicide/$SHA-egdb7; mkdir -p $D7
cp /mnt/c/regicide-match/engines/regicide/$SHA-egdb8/regicide_cb.dll $D7/
[ -d $D7/egdb ] || cp -r /mnt/c/regicide-match/engines/regicide/30f09c3/egdb $D7/egdb
echo "egdb7 files: $(ls $D7/egdb | wc -l), $(du -sh $D7/egdb | cut -f1)"
CB=/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe
for d in $SHA-egdb7 $SHA-egdb8; do
  echo "-- cbhost $d"
  printf 'cmd get dbpieces\ncmd set dbmbytes 12288\ncmd get dbpieces\ncmd set avoidfile C:\\regicide-match\\avoid\\avoid_v1.bin\nquit\n' | \
    $CB "C:\\regicide-match\\engines\\regicide\\$d\\regicide_cb.dll" 2>&1 | tr -d '\r'
done
echo "BUILD_769_DONE $(date -Is)" >> ~/regicide-jobs/r4/status
