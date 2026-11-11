#!/usr/bin/env bash
# reachability check of r11b (witnesses from theory_r11b.sqlite: the games the pins come from)
L=~/regicide-jobs/booklab; W=~/regicide-booklab; cd $L
nice python3 $W/tools/booklab/reach_check.py --decisions decisions_r11b.txt --book book_r11b.bin --reach $W/build-linux/booklab_reach \
  --db theory_r11b.sqlite --limit 200000 --egdb ~/regicide-data/egdb > book_r11b.reach.txt 2>&1
echo "REACH r11b done: $(tail -1 book_r11b.reach.txt) $(date -Is)" >> status
