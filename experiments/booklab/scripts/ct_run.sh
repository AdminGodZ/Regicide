#!/usr/bin/env bash
# WP9 step 2: conversion_test with the real 2-8-piece database (as the lead specified), split into 4 processes
L=~/regicide-jobs/booklab; mkdir -p $L/ct
cd $L
for p in 7 8; do for r in win lose; do
  nice bin/blk-conversion_test --dir ~/regicide-data/egdb --pieces $p --kind both --sample 200 --cap 32 --nodes 100000 \
    --oracle-nodes 1000000 --role $r --list > ct/p${p}_$r.log 2>&1 &
done; done
wait
echo "CT done $(date -Is)" >> $L/status
