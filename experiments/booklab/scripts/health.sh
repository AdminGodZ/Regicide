#!/usr/bin/env bash
# Health of timed runs (lead 2026-11-11: a second reviewer shares the machine; report timing drift, losses on time
# or a failing free-memory check at once). Since the line count in $L/health.pos: status lines with STILL LOW, CRASHES,
# INCOMPLETE, refused, STOPPED; games ending "lost on time" / "timed out" / crashes in runs finished since
# (CheckerBoard banking lets single moves take several seconds, so move times are not a drift signal.) Prints "HEALTH OK" or the problems.
L=~/regicide-jobs/booklab; R=~/regicide-match/results
pos=$(cat $L/health.pos 2>/dev/null || echo 0); n=$(wc -l < $L/status)
new=$(tail -n +$((pos + 1)) $L/status | head -n $((n - pos)))
echo $n > $L/health.pos
bad=$(echo "$new" | grep -E "STILL LOW|CRASHES|INCOMPLETE|refused|STOPPED")
for t in $(echo "$new" | sed -n 's/^BL end \([^ ]*\) .*/\1/p'); do
  f=$R/bl-$t/games.pdn; [ -f $f ] || continue
  c=$(grep -c -E 'Termination "[^"]*(lost on time|timed out|crashed|did not start)' $f)
  [ "$c" != 0 ] && bad="$bad
$t: $c games lost on time / timed out"
  echo "checked $t"
done
[ -z "$bad" ] && echo "HEALTH OK" || { echo "HEALTH PROBLEM:"; echo "$bad"; }
