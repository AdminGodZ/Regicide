#!/usr/bin/env bash
# wind down (lead, 2026-11-11 ~16:40): stop n9a_variant before it starts, both generators (whole records kept),
# pausekeeper and keepalive; no locks left.
source /mnt/c/regicide-match/jobs/truth/common.sh
O=~/regicide-data/truth
for p in $(pgrep -f "regicide-jobs/truth/n9a_variant.sh"); do kill $p; done
st "wind-down: n9a_variant stopped before n9a0 started"
alive() {
  for p in $(pgrep -x "datagen-tr[56]") $(pgrep -f "regicide-jobs/truth/e5_gen.sh"); do
    [ "$(awk '/^State/{print $2}' /proc/$p/status 2>/dev/null)" != Z ] && [ -e /proc/$p ] && return 0
  done
  return 1
}
for i in $(seq 1 60); do
  alive || break
  for p in $(pgrep -x "datagen-tr[56]") $(pgrep -f "regicide-jobs/truth/e5_gen.sh"); do kill -TERM $p 2>/dev/null; kill -CONT $p 2>/dev/null; done
  sleep 1
done
alive && echo "GENERATORS STILL ALIVE"
for f in gen6a_n50k_egdb gen6b_n50k_egdb; do
  s=$(stat -c %s $O/$f.bin); w=$(( s / 16 * 16 ))
  [ $s -ne $w ] && truncate -s $w $O/$f.bin
  echo "$f: $(( w / 16 )) records (cut $(( s - w )) bytes of a partial record)"
done
rm -f $LOCKS/pk_hold $LOCKS/yield_truth
sleep 5
rm -f ~/regicide-jobs/truth/hold
for p in $(pgrep -f "jobs/truth/keepalive.sh"); do kill $p; done
st "wind-down: generators stopped, files whole; pausekeeper and keepalive stopped"
sleep 4
echo "== left:"; pgrep -af "datagen|e5_gen|pausekeeper_truth|keepalive|trigger|n9a_variant|train.py|truth-tr|n9.sh" | grep -v winddown || echo none
ls $LOCKS; flock -n $LOCKS/timing.lock true && echo "timing.lock not held" || echo "timing.lock held (by someone)"
fuser $LOCKS/timing.lock 2>&1 | head -2
