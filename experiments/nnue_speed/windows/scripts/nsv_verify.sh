#!/usr/bin/env bash
# Phase 1 of the t3/nnue-speed verification: waits for the ae210fb baseline and the nn3e work (EXTRA_DONE), pauses
# datagen (stays paused for phase 2), runs experiments/nnue_speed/scripts/verify_x86.sh on a 100k-record slice of the
# round-3 validation set.
J=~/regicide-jobs/r3
st() { echo "$* $(date -Is)" >> $J/status; }
until grep -q '^EXTRA_DONE' $J/status; do sleep 60; done
N=~/regicide-nsv
git -C ~/regicide-nnue fetch -q origin
[ -d $N ] || git -C ~/regicide-nnue worktree add -q --detach $N origin/t3/nnue-speed
cd $N && git checkout -q --detach origin/t3/nnue-speed
st "NSV checkout $(git log --oneline -1)"
SLICE=~/regicide-nnue3-work/nsv_valid_100k.bin
head -c $((16 * 100000)) ~/regicide-nnue3-work/nnue_valid_1M.bin > $SLICE
touch $J/matchlock
DG=$(pgrep -f '[d]atagen-(825f993|ae210fb)' | tr '\n' ' ')
[ -n "$DG" ] && kill -STOP $DG && st "NSV paused datagen $DG"
echo "$DG" > $J/nsv_paused_pids
sleep 3
J=16 bash experiments/nnue_speed/scripts/verify_x86.sh $SLICE > $J/nsv_verify.out 2>&1
echo "rc=$?" >> ~/regicide-jobs/r3/nsv_verify.out
J=~/regicide-jobs/r3
st "NSV_VERIFY_DONE (datagen still paused)"
