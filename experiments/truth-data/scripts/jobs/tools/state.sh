date
for f in ~/regicide-data/truth/gen6a_n50k_egdb.bin ~/regicide-data/truth/gen6b_n50k_egdb.bin; do echo "$(basename $f) $(( $(stat -c %s $f) / 16 ))"; done
echo "total $(( ( $(stat -c %s ~/regicide-data/truth/gen6a_n50k_egdb.bin) + $(stat -c %s ~/regicide-data/truth/gen6b_n50k_egdb.bin) ) / 16 ))"
for p in $(pgrep -x datagen-tr5) $(pgrep -x datagen-tr6); do echo "$(ps -o comm=,stat= -p $p) cpu $(awk '{print ($14+$15)/100}' /proc/$p/stat)"; done
ls -la ~/regicide-jobs/r4/matchlock ~/regicide-jobs/locks/yield_truth 2>&1 | cut -c1-90
grep -vE "^(TR|PKT)" ~/regicide-jobs/r4/status | tail -n 3 | cut -c1-150
grep -E "^PKT" ~/regicide-jobs/r4/status | tail -n 2 | cut -c1-120
cd ~/regicide-truth && git fetch -q origin && git log --oneline -1 origin/master && git log --oneline -1
