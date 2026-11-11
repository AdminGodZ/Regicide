#!/usr/bin/env bash
# 2c (lead 2026-11-11): r11d (DLL bl-1c6e997, master 1c6e997) against Cake strictly in match order: concurrency 1,
# --pairs-together, processes kept, the whole production deck (312 games), PDN comments on. One game at a time, so it
# runs without the timing lock in the background of everything else (NOLOCK; its start line says so). After 2b.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
until grep -q "MO 2b done\|MO STOPPED" $L/status; do sleep 60; done
grep -q "MO STOPPED" $L/status && ! grep -q "MO 2b done" $L/status && { st "MO1 not started (2b stopped)"; exit 1; }
export MATCH=$L/bin/match-1c6e CBHOST=$L/bin/1c6e/cbhost.exe
NOLOCK=1 EXTRA_ARGS="--allow-rejected-options --pairs-together" bash $J/bl_run_w.sh regicide:bl-1c6e997 cake mo1-cake $J/acf156_all.txt 0.5 1 < /dev/null \
  && st "MO1 done" || st "MO1 STOPPED"
