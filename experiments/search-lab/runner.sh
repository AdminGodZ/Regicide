#!/bin/bash
# search-lab queue runner. Queue lines: name|binary (in bin/)|A params|movetime ms|pairs|B params|extra selfplay args
# Plays lines not yet in done.txt or failed.txt, in order. A line is done only if selfplay exited 0 and its "final"
# report has 2 x pairs games and a pentanomial summing to the queued pairs. Otherwise the log is kept as
# logs/<name>.failed-<epoch>.log, the line goes to failed.txt (not retried automatically). Exits when STOP exists.
U=~/regicide-jobs/searchlab
echo "RUNNER START $(date -Is)"
touch $U/failed.txt
while [ ! -f $U/STOP ]; do
  line=$(cat $U/done.txt $U/failed.txt | /usr/bin/grep -avxFf - $U/queue.txt | /usr/bin/grep -av '^#' | /usr/bin/grep -a . | head -1)
  if [ -z "$line" ]; then echo "IDLE $(date -Is)"; sleep 60; continue; fi
  IFS='|' read -r name bin a mt pairs b extra <<<"$line"
  echo "BEGIN $name bin=$bin a=\"$a\" b=\"$b\" mt=$mt pairs=$pairs extra=\"$extra\" $(date -Is)"
  s=$(date +%s)
  "$U/bin/$bin" --a "$a" --b "$b" --movetime "$mt" --threads 16 --pairs "$pairs" $extra > "$U/logs/$name.log" 2>&1
  rc=$?
  fin=$(/usr/bin/grep -a '^final' $U/logs/$name.log)
  ok=$(python3 -I - "$fin" "$pairs" <<'PY'
import re, sys
fin, pairs = sys.argv[1], int(sys.argv[2])
m = re.search(r'final games (\d+) .*penta \[(\d+) (\d+) (\d+) (\d+) (\d+)\]', fin)
print('yes' if m and int(m.group(1)) == 2 * pairs and sum(map(int, m.groups()[1:])) == pairs else 'no')
PY
)
  if [ $rc = 0 ] && [ "$ok" = yes ]; then
    echo "DONE $name rc=$rc secs=$(( $(date +%s)-s )) $(date -Is) :: $fin"
    echo "$line" >> $U/done.txt
  else
    f=$U/logs/$name.failed-$(date +%s).log; mv $U/logs/$name.log $f
    echo "FAILED $name rc=$rc complete=$ok secs=$(( $(date +%s)-s )) $(date -Is) :: log kept as $f"
    [ -f $U/STOP ] && [ $rc != 0 ] || echo "$line" >> $U/failed.txt
  fi
done
echo "STOPPED $(date -Is)"
