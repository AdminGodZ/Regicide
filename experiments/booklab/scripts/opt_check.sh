#!/usr/bin/env bash
# Item 4 (lead 2026-11-11 14:32): every option line our match recipe sends to each opponent DLL (bl_run_w.sh), sent
# through the same host (fdd154e) one by one with book_probe.py --show-replies, each followed by "get <name>" to read
# the value back; "reply 1 ..." = accepted, "reply 0 ..." = rejected (the runs use --allow-rejected-options, so a
# rejected line was silently ignored). Under the timing lock (loading a DLL maps its database).
L=~/regicide-jobs/booklab; CB=$L/bin/fdd1/cbhost.exe; P=/mnt/c/regicide-match/jobs/booklab/src/tools/booklab/book_probe.py
E='C:\regicide-match\engines'; DB8='C:\regicide-match\db\kingsrow_wld8'; OUT=$L/cl/opponent_options.md
exec 9>~/regicide-jobs/locks/timing.lock; flock 9
{
echo "# Opponent options: what each opponent DLL answers to our recipe's option lines"
echo
echo "Host: cbhost.exe from master fdd154e. Each engine in one fresh process; the lines in the order of bl_run_w.sh,"
echo "each followed by \"get <name>\". Raw replies below; \"reply 1\" = accepted, \"reply 0\" = rejected."
for spec in "Cake 1.89f|$E\\cake\\cake_189f.dll|" "KingsRow 1.20|$E\\kingsrow\\Kingsrow64.dll|kr" "GuiNN 2.06|$E\\guinn\\gui_nn64.dll|"; do
  IFS='|' read -r name dll kr <<< "$spec"
  args=()
  for o in "set dbpath $DB8" "set enable_wld 1" "set dbmbytes 512" "set max_dbpieces 8" "set hashsize 128"; do
    args+=(--opt "$o" --opt "get ${o#set }"); args[-1]="get $(echo ${o#set } | cut -d' ' -f1)"
  done
  [ -n "$kr" ] && args+=(--opt "set searchthreads 1" --opt "get searchthreads")
  echo; echo "## $name"; echo; echo '```'
  python3 $P --cbhost $CB --dll "$dll" "${args[@]}" --book 3 --readback-only --show-replies 2>&1
  echo '```'
done
} > $OUT
flock -u 9
cat $OUT
