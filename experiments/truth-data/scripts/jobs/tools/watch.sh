# watch.sh SECONDS: wait up to SECONDS for a phase-3 event (E5 >= 15M, losswalk done, master changed), then report
end=$(( $(date +%s) + $1 )); m0=$(cd ~/regicide-truth && git ls-remote origin refs/heads/master | cut -c1-7)
tot() { echo $(( ( $(stat -c %s ~/regicide-data/truth/gen6a_n50k_egdb.bin) + $(stat -c %s ~/regicide-data/truth/gen6b_n50k_egdb.bin) ) / 16 )); }
while [ $(date +%s) -lt $end ]; do
  [ $(tot) -ge 17200000 ] && break
  grep -q "LOSSWALK_DONE" ~/regicide-jobs/truth/status && [ ! -e /tmp/lw_reported ] && { touch /tmp/lw_reported; break; }
  sleep 60
done
m1=$(cd ~/regicide-truth && git ls-remote origin refs/heads/master | cut -c1-7)
echo "$(date +%H:%M) E5 $(tot) | master $m1 | $(grep -c 'losswalk ' ~/regicide-jobs/truth/status) losswalk lines | last: $(grep '^TR losswalk' ~/regicide-jobs/truth/status | tail -n 1 | cut -c4-60)"
