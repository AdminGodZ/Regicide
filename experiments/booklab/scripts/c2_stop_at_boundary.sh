#!/usr/bin/env bash
# Wind-down (lead 2026-11-11): stop the WP12 c2 scoring at a resumable point. wp12_select.py appends a batch's move
# scores to wp12/cache.tsv when its multiscore call ends; a restart of c2_chain.sh recomputes the same points and uses
# every cached score (same campaign key). Waits up to MAX minutes for the running batch to end (cache grows), then
# stops wp12_select.py, its multiscore and the c2 chain; reports whether the in-flight batch was saved or lost.
L=~/regicide-jobs/booklab; MAX=${1:-15}
st() { echo "$* $(date -Is)" >> $L/status; }
n0=$(grep -c "^8f5621a2" $L/wp12/cache.tsv)
saved=no
for i in $(seq $((MAX * 6))); do
  [ "$(grep -c "^8f5621a2" $L/wp12/cache.tsv)" -gt "$n0" ] && { saved=yes; sleep 5; break; }
  pgrep -f "wp12_select.py" > /dev/null || { saved=finished; break; }
  sleep 10
done
for p in $(pgrep -f "c2_chain.sh|wp12_select.py|blk-multiscore-1c6e"); do kill $p 2>/dev/null; done
sleep 2
n1=$(grep -c "^8f5621a2" $L/wp12/cache.tsv)
st "W12C2 stopped for the night (batch boundary reached: $saved; cached move scores $n0 -> $n1); resume: launch.sh c2 c2_chain.sh"
echo "boundary reached: $saved; cached move scores $n0 -> $n1"
pgrep -fa "c2_chain|wp12_select|blk-multiscore" | cut -c1-80
true
