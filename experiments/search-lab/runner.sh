#!/bin/bash
# search-lab queue runner. Queue lines: name|binary (in bin/)|A params|movetime ms|pairs|B params|extra selfplay args
# Plays lines not yet in done.txt, in order. Exits after the current test when STOP exists.
U=~/regicide-jobs/searchlab
echo "RUNNER START $(date -Is)"
while [ ! -f $U/STOP ]; do
  line=$(/usr/bin/grep -avxFf $U/done.txt $U/queue.txt | /usr/bin/grep -av '^#' | /usr/bin/grep -a . | head -1)
  if [ -z "$line" ]; then echo "IDLE $(date -Is)"; sleep 60; continue; fi
  IFS='|' read -r name bin a mt pairs b extra <<<"$line"
  echo "BEGIN $name bin=$bin a=\"$a\" b=\"$b\" mt=$mt pairs=$pairs extra=\"$extra\" $(date -Is)"
  s=$(date +%s)
  "$U/bin/$bin" --a "$a" --b "$b" --movetime "$mt" --threads 16 --pairs "$pairs" $extra > "$U/logs/$name.log" 2>&1
  rc=$?
  echo "DONE $name rc=$rc secs=$(( $(date +%s)-s )) $(date -Is) :: $(/usr/bin/grep -a '^final' $U/logs/$name.log)"
  echo "$line" >> $U/done.txt
done
echo "STOPPED $(date -Is)"
