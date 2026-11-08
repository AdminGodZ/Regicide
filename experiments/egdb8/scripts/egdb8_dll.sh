#!/usr/bin/env bash
# CheckerBoard engine for the 8-piece baseline: the same 30f09c3 DLL as the 7-piece baseline (play-identical to
# eb1037c-n4c), with a full 2-8 piece copy of ~/regicide-data/egdb in its egdb/ folder; then check through cbhost.exe
# that it loads 8 pieces, and how long the first command (which loads the database) takes.
set -u
JE=~/regicide-jobs/egdb8; N=30f09c3-egdb8
SRC=/mnt/c/regicide-match/engines/regicide/30f09c3; DST=/mnt/c/regicide-match/engines/regicide/$N
mkdir -p $DST
cp $SRC/regicide_cb.dll $DST/
cmp $SRC/regicide_cb.dll $DST/regicide_cb.dll && echo "DLL = engines/regicide/30f09c3/regicide_cb.dll"
t0=$(date +%s)
mkdir -p $DST/egdb
for f in ~/regicide-data/egdb/*.rgdb; do b=$(basename $f); [ -e $DST/egdb/$b ] && [ $(stat -c %s $f) = $(stat -c %s $DST/egdb/$b) ] || cp $f $DST/egdb/; done
echo "copy took $(( $(date +%s) - t0 )) s"
diff -rq --exclude=parts ~/regicide-data/egdb $DST/egdb && echo "egdb copy identical (2-8 pieces, $(ls $DST/egdb | wc -l) files, $(du -sh $DST/egdb | cut -f1))"
{ cat $SRC/BUILD.txt | grep -v '^egdb:'; echo "egdb: full copy of ~/regicide-data/egdb (2-8 pieces; 8-piece WLD built 2026-11-08 with egdb_gen 35ae5ea) in egdb/"; } > $DST/BUILD.txt
cat $DST/BUILD.txt
CB=/mnt/c/regicide-match/bin/b71dfa9/cbhost.exe
W="C:\\regicide-match\\engines\\regicide\\$N\\regicide_cb.dll"
for i in 1 2; do
  t0=$(date +%s.%N)
  printf 'cmd get dbpieces\ncmd get dbpath\ncmd get dbmbytes\nquit\n' | $CB "$W" 2>&1 | tr -d '\r'
  echo "cbhost run $i: $(echo "$(date +%s.%N) - $t0" | bc) s"
done
echo "EGDB8DLL_DONE $(date -Is)" >> $JE/status
