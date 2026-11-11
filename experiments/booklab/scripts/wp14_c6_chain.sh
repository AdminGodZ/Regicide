#!/usr/bin/env bash
# WP14 item 0 (lead 2026-11-11 13:56): test of reviewer B's #6 Black repair, book r11d with the ply-17 position
# B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14 forced to 8-11 instead of 6-10 (one-key override of the built-in
# book of master fdd154e; DLL bl-w12-wp14-c6-811, book sha256 76cc9a75...). ACF #6 Inferno, Regicide Black from the
# ballot; concurrency 8; runner and host from fdd154e; set book 3 read back; claims recorded.
#   KingsRow mode 3: 24 fresh, 24 kept; Cake: 24 fresh, 24 kept; GuiNN: 12 fresh, 12 kept  (tags w14c6-<opp>-<arm>)
# Waits for WP14 step 1 (320 result lines) so that no search of mine runs beside the timed games, and pauses the WP12
# c2 scoring (SIGSTOP of wp12_select.py and blk-multiscore-1c6e) for the duration; resumes it at the end.
J=/mnt/c/regicide-match/jobs/booklab; L=~/regicide-jobs/booklab
st() { echo "$* $(date -Is)" >> $L/status; }
until [ "$(wc -l < $J/wp14/step1.out)" -ge 320 ]; do sleep 30; done
pause() { for p in $(pgrep -f "wp12_select.py|blk-multiscore-1c6e"); do kill -$1 $p 2>/dev/null; done; }
pause STOP; st "W14C6 start (c2 scoring paused)"
trap 'pause CONT; st "W14C6 end (c2 scoring resumed)"' EXIT
export MATCH=$L/bin/match-fdd1 CBHOST=$L/bin/fdd1/cbhost.exe
mkdir -p $L/wp14
: > $L/wp14/c6_24.txt; for i in $(seq 24); do echo "| 9-13 22-18 10-14 | A=B" >> $L/wp14/c6_24.txt; done
head -12 $L/wp14/c6_24.txt > $L/wp14/c6_12.txt
for spec in "kingsrow 24" "cake 24" "guinn 12"; do
  set -- $spec
  for arm in fresh kept; do
    X="--allow-rejected-options"; [ $arm = fresh ] && X="$X --fresh-engines"
    pause STOP
    EXTRA_ARGS="$X" bash $J/bl_run_w.sh regicide:bl-w12-wp14-c6-811 $1 w14c6-$1-$arm $L/wp14/c6_$2.txt 0.5 8 < /dev/null \
      || { st "W14C6 STOPPED at w14c6-$1-$arm"; exit 1; }
  done
done
st "W14C6 done"
