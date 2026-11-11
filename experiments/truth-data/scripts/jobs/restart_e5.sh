#!/usr/bin/env bash
# restart_e5.sh SHA_PREFIX: stop both E5 generators, keep whole records, install ~/regicide-truth/build/datagen as
# datagen-tr6 / datagen-tr5 (sha256 must start with SHA_PREFIX), restart them with --avoid and parities 0 / 1
# (gen6a seed 618, gen6b seed 619). Waits (up to 3 h) until the timing lock is free and holds it during the restart, so
# no Book Lab match is running meanwhile; the hold is short (well under 15 minutes).
source /mnt/c/regicide-match/jobs/truth/common.sh
NEW=$1; O=~/regicide-data/truth; T=/mnt/c/regicide-match/jobs/truth
[ -n "$NEW" ] || { echo "usage: restart_e5.sh SHA_PREFIX"; exit 2; }
sha=$(sha256sum ~/regicide-truth/build/datagen | cut -c1-64)
case $sha in $NEW*) ;; *) st "e5 restart NOT done: build/datagen has sha256 ${sha:0:16}, expected $NEW"; exit 1 ;; esac
exec 9> $LOCKS/timing.lock
st "e5 restart: waiting for the timing lock"
flock -w 10800 9 || { st "e5 restart NOT done: timing lock busy for 3 h"; exit 1; }  # blocking: takes the first gap
st "e5 restart: timing lock taken (free between Book Lab runs)"
# The pausekeeper stops the yield group (datagen-tr5 and its bash parent) every 3 s while Book Lab computes or the
# timing lock is held (as now): a stopped parent cannot reap its child (2026-11-11 02:32: a zombie datagen-tr5 blocked
# the first attempt). So the e5_gen.sh parents get TERM + CONT as well, repeatedly, and zombies do not count.
alive() {  # running or stopped (not zombie) generators or their e5_gen.sh parents
  for p in $(pgrep -x "datagen-tr[56]") $(pgrep -f "regicide-jobs/truth/e5_gen.sh"); do
    [ "$(awk '/^State/{print $2}' /proc/$p/status 2>/dev/null)" != Z ] && [ -e /proc/$p ] && return 0
  done
  return 1
}
for i in $(seq 1 60); do
  alive || break
  for p in $(pgrep -x "datagen-tr[56]") $(pgrep -f "regicide-jobs/truth/e5_gen.sh"); do
    kill -TERM $p 2>/dev/null; kill -CONT $p 2>/dev/null
  done
  sleep 1
done
if alive; then
  flock -u 9; st "e5 restart FAILED: old generators still running"; exit 1
fi
for f in gen6a_n50k_egdb gen6b_n50k_egdb; do
  s=$(stat -c %s $O/$f.bin); w=$(( s / 16 * 16 ))
  [ $s -ne $w ] && truncate -s $w $O/$f.bin
  echo "$f $(( w / 16 )) records (cut $(( s - w )) bytes)" >> $J/restart_e5.log
  echo "$f $(( w / 16 ))" > $J/restart_offset_$f.txt
done
cp ~/regicide-truth/build/datagen $J/bin/datagen-tr6 && cp ~/regicide-truth/build/datagen $J/bin/datagen-tr5 || {
  flock -u 9; st "e5 restart FAILED: cannot install the binary"; exit 1; }
cd $T
EXPECT_SHA=$NEW bash launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 618 6 0 gen6b_n50k_egdb 9>&-
EXPECT_SHA=$NEW bash launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 619 6 1 gen6a_n50k_egdb 9>&-
sleep 60   # the preload reads both files
grep -h "avoid:\|start parity" $J/e5_gen6a_n50k_egdb.log $J/e5_gen6b_n50k_egdb.log | tail -4 >> $J/restart_e5.log
flock -u 9
st "e5 restart done ($(pgrep -x datagen-tr6 | wc -l)+$(pgrep -x datagen-tr5 | wc -l) generators; timing lock released)"
