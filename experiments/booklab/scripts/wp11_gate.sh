#!/usr/bin/env bash
# Gate between WP10 block 4 and the WP11 validation: wp11_chain.sh polls every 20 s and only stops a block-5 run that
# still waits for the lock. This holds the timing lock from the end of the last block-4 run until wp11_chain.sh has
# taken over, so block 5 cannot start first. Holds nothing while it waits (the lock is idle for <= ~25 s).
L=~/regicide-jobs/booklab; LOCK=~/regicide-jobs/locks/timing.lock
until grep -q "BL start w10-b4-EB-guinn-H2x_1" $L/status; do sleep 10; done
exec 9>$LOCK
flock 9
echo "WP11 gate: holding the timing lock until the WP11 validation starts $(date -Is)" >> $L/status
until grep -q "WP11 validation start" $L/status; do sleep 2; done
sleep 3
flock -u 9
echo "WP11 gate: released $(date -Is)" >> $L/status
