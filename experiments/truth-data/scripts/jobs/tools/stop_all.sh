J=~/regicide-jobs/truth; L=~/regicide-jobs/locks; O=~/regicide-data/truth
# watchers and chains first (so nothing restarts anything)
for pat in "regicide-jobs/truth/n9h_trigger.sh" "regicide-jobs/truth/losswalk.sh" "regicide-jobs/truth/e5_gen.sh" "jobs/truth/e5_gen.sh" "after_gen4a.sh" "regicide-jobs/truth/n9.sh"; do
  for p in $(pgrep -f "$pat"); do kill $p 2>/dev/null; done; done
# pausekeeper: end its loop and resume anything it had stopped
rm -f $L/pk_hold $L/yield_truth
for p in $(pgrep -f "regicide-jobs/truth/pausekeeper_truth.sh"); do kill $p; done
pkill -CONT -x datagen-tr5; pkill -CONT -x datagen-tr6; pkill -CONT -x truth-tr4
sleep 1
pkill -x truth-tr4; pkill -x datagen-tr5; pkill -x datagen-tr6; pkill -f "tools/nnue/train.py" 2>/dev/null
sleep 4
echo "left of mine:"; pgrep -af "datagen-tr|truth-tr|rescore-tr|regicide-jobs/truth/|jobs/truth/|train.py" | grep -v pgrep | cut -c1-100
for f in gen6a_n50k_egdb gen6b_n50k_egdb; do s=$(stat -c %s $O/$f.bin); r=$((s % 16)); [ $r -ne 0 ] && truncate -s $((s - r)) $O/$f.bin; echo "$f: $(stat -c %s $O/$f.bin) bytes, mod16 was $r, records $(( $(stat -c %s $O/$f.bin) / 16 ))"; done
ls -la ~/regicide-jobs/r4/matchlock $L/timing.lock 2>&1 | cut -c1-100
echo "TR STOPPED for machine restart (owner) $(date -Is)" | tee -a ~/regicide-jobs/r4/status >> $J/status
ls $J/lw_*.tsv | wc -l
