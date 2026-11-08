#!/usr/bin/env bash
# nn1 vs bd2ef1d built-in, same bd2ef1d regicide_cb.so on both sides; gen3a datagen paused for the match.
N=~/regicide-nnue-work
cp ~/regicide-nnue/build/libregicide_cb.so $N/engines/rbd2ef1d.so
DG=$(pgrep -f '[d]atagen-bd2ef1d')
[ -n "$DG" ] && kill -STOP $DG && echo "paused gen3a datagen $DG for match $(date -Is)" >> $N/status
sleep 2
bash ~/regicide-jobs/a44591e/nnue_match.sh nn1 $N/engines/rbd2ef1d.so bd2ef1d $N/engines/rbd2ef1d.so --option-a "set evalfile $N/nn1.nnue"
[ -n "$DG" ] && kill -CONT $DG && echo "resumed gen3a datagen $DG $(date -Is)" >> $N/status
