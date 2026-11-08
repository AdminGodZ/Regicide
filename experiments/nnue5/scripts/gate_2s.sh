#!/usr/bin/env bash
# Hold ~/regicide-jobs/r3/matchlock (which r5_chain.sh waits on before bench + matches) until the 2 s KingsRow run has
# really finished (its rc=0 end line; the earlier "kingsrow-2s rc=1" line is from the failed 11:31 launch).
J=~/regicide-jobs/r4; L=~/regicide-jobs/r3/matchlock
st() { echo "R5 $* $(date -Is)" >> $J/status; }
grep -q '^NSVC_DONE' ~/regicide-jobs/r3/status || { st "gate_2s: r3 not done, not touching its lock"; exit 1; }
[ -e $L ] && { st "gate_2s: r3 matchlock already exists, leaving it"; exit 1; }
touch $L; st "gate_2s: holding r3/matchlock until the 2 s run ends"
until grep -q 'BASELINE end eb1037c-n4c kingsrow-2s rc=0' $J/status; do sleep 30; done
rm -f $L; st "gate_2s: 2 s run over, released r3/matchlock"
