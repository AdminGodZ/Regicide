#!/usr/bin/env bash
# WP9: conversion test. Plays every deck in POSDIR (one game per position, Regicide = engine A with the side that has
# the database win, "A=B"/"A=W" opening field) against the deck's opponent, 0.5 s/move, baseline settings, one
# timing-lock hold per deck (<= 40 positions), then writes the report.
#   wp9_run.sh DLLDIR TAG POSDIR
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
DLL=$1; TAG=$2; P=$(readlink -f $3)
export MATCH=$L/bin/match-bl
echo "WP9 $TAG start dll $DLL positions $P $(date -Is)" >> $L/status
DIRS=()
for d in $P/*_[0-9]*.txt; do
  n=$(basename $d .txt); opp=${n%_*}
  bash $J/bl_run.sh "regicide:$DLL" $opp "$TAG-$n" $d 0.5 8 < /dev/null
  DIRS+=(~/regicide-match/results/bl-$TAG-$n)
done
python3 $J/src/tools/booklab/wp9.py report --positions $P/positions.tsv --scan $L/bin/blk-dbscan \
  --egdb ~/regicide-data/egdb --out $L/wp9/report-$TAG.md "${DIRS[@]}" > $L/wp9/report-$TAG.log 2>&1
echo "WP9 $TAG done $(grep -m1 Converted $L/wp9/report-$TAG.md) $(date -Is)" >> $L/status
sleep 3
