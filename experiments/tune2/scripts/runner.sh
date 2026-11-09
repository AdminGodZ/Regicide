#!/bin/bash
# tune2 queue runner (base 6bb8c6c). Waits for the deep-book job to finish. Stops on STOP.
U=~/regicide-jobs/tune2
while systemctl --user is-active -q regicide-bookdeep || systemctl --user is-active -q regicide-bookdeep-topup; do sleep 10; done
echo "RUNNER START (base 6bb8c6c) $(date -Is)"
while [ ! -f $U/STOP ]; do
  line=$(/usr/bin/grep -avxFf $U/done.txt $U/queue.txt | /usr/bin/grep -av '^#' | /usr/bin/grep -a . | head -1)
  if [ -z "$line" ]; then echo "IDLE $(date -Is)"; sleep 60; continue; fi
  IFS='|' read -r name a mt pairs b extra <<<"$line"
  echo "BEGIN $name a=\"$a\" b=\"$b\" mt=$mt pairs=$pairs extra=\"$extra\" $(date -Is)"
  s=$(date +%s)
  "$U/bin/selfplay-6bb8c6c" --a "$a" --b "$b" --movetime "$mt" --threads "${THREADS:-16}" --pairs "$pairs" $extra > "$U/logs/$name.log" 2>&1
  rc=$?
  echo "DONE $name rc=$rc secs=$(( $(date +%s)-s )) $(date -Is) :: $(/usr/bin/grep -a '^final' $U/logs/$name.log)"
  echo "$line" >> $U/done.txt
done
echo "STOPPED $(date -Is)"
