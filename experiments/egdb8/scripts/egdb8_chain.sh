#!/usr/bin/env bash
# 8-piece EGDB build (approved by the user 2026-11-08), into ~/regicide-data/egdb (adds wld_*.rgdb 8-piece files):
#   egdb_gen build --max 8 --threads 8 --sides 44, then --sides 53, then --max 8 (the rest). Resumable: rerun.
# gen4b datagen is paused while ~/regicide-jobs/r4/egdblock exists (pausekeeper2.sh). Afterwards:
#   egdb_gen verify --pieces 8 --samples 50000 (every 8-piece slice), egdb_gen manifest, egdb_test --data.
J=~/regicide-jobs/r4; JE=~/regicide-jobs/egdb8; D=~/regicide-data/egdb; G=$JE/bin/egdb_gen
st() { echo "EGDB8 $* $(date -Is)" | tee -a $J/status >> $JE/status; }
touch $J/egdblock
st "chain start pid $$, egdb_gen $(cat $JE/bin/SHA), free $(df -B1G --output=avail ~ | tail -n 1 | tr -d ' ') GB"
ls $D | sort > $JE/files_before.txt
run() {  # run LOGNAME ARGS...
  local L=$1; shift
  if grep -q '^EGDB8 end '"$L"' rc=0' $JE/status 2>/dev/null; then return 0; fi
  st "start $L: egdb_gen build --max 8 --threads 8 $* --dir $D"
  local t0=$(date +%s)
  /usr/bin/time -v $G build --max 8 --threads 8 "$@" --dir $D > ~/regicide-data/$L.log 2> $JE/$L.time.txt
  local rc=$?
  st "end $L rc=$rc $(( $(date +%s) - t0 )) s, maxrss $(grep 'Maximum resident' $JE/$L.time.txt | awk '{print $NF}') kB, db $(du -sh $D | cut -f1), free $(df -B1G --output=avail ~ | tail -n 1 | tr -d ' ') GB"
  return $rc
}
run gen8_44 --sides 44 && run gen8_53 --sides 53 && run gen8_rest
RC=$?
rm -f $J/egdblock
st "BUILD_DONE rc=$RC (gen4b datagen resumed by pausekeeper2)"
[ $RC -eq 0 ] || exit $RC
ls $D | sort > $JE/files_after.txt
comm -13 $JE/files_before.txt $JE/files_after.txt > $JE/files_new.txt
(cd $D && ls -l $(cat $JE/files_new.txt)) > $JE/sizes_new.txt 2>&1
st "new files: $(wc -l < $JE/files_new.txt), $(awk '{s+=$5} END {printf "%.2f GB", s/1e9}' $JE/sizes_new.txt)"
t0=$(date +%s)
$G verify --pieces 8 --samples 50000 --threads 8 --dir $D > $JE/verify8.txt 2>&1; RV=$?
st "verify rc=$RV $(( $(date +%s) - t0 )) s: $(tail -n 1 $JE/verify8.txt)"
$G manifest --dir $D > $JE/manifest_after.txt 2>&1; RM=$?
st "manifest rc=$RM $(grep -c ' ok$' $JE/manifest_after.txt) ok lines, $(grep -vc ' ok$' $JE/manifest_after.txt) other"
REGICIDE_EGDB=$D $JE/bin/egdb_test --data > $JE/egdb_test_data_after.txt 2>&1
st "egdb_test --data rc=$? $(grep -m1 'max_pieces' $JE/egdb_test_data_after.txt)"
st "EGDB8_DONE"
