#!/usr/bin/env bash
# Hold the WSL VM up while ~/regicide-jobs/r4/hold exists (round-4 jobs run detached with setsid nohup).
J=~/regicide-jobs/r4; mkdir -p $J; touch $J/hold
echo "keepalive_r4 start $(date -Is) pid $$" >> $J/status
while [ -e $J/hold ]; do sleep 60; done
echo "keepalive_r4 end $(date -Is)" >> $J/status
