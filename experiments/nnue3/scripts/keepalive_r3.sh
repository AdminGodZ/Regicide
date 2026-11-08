#!/usr/bin/env bash
# Hold the WSL VM up while the round-3 hold file exists or any datagen / trainer / match process runs.
J=~/regicide-jobs/r3
mkdir -p $J
echo "keepalive_r3 start $(date -Is) pid $$" >> $J/status
sleep 30
while [ -e $J/hold ] || pgrep -f '[d]atagen-' > /dev/null || pgrep -f '[t]ools/nnue/train.py' > /dev/null || pgrep -f '[r]3_chain' > /dev/null; do sleep 60; done
echo "keepalive_r3 end $(date -Is)" >> $J/status
