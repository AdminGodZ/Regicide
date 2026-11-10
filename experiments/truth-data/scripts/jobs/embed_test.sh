#!/usr/bin/env bash
# embed_test.sh NAME: build the engine with NAME.nnue as the built-in net (src/weights/net.bin) in a scratch worktree
# and run ctest + bench. Does not touch the branch.
source /mnt/c/regicide-match/jobs/truth/common.sh
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
n=$1; D=/tmp/rg-embed-$n
while [ -e $R4/matchlock ]; do sleep 15; done
touch $LOCKS/yield_truth
cd ~/regicide-truth && rm -rf $D && git worktree add -q --detach $D HEAD
cp ~/regicide-truth-work/$n.nnue $D/src/weights/net.bin
cmake -S $D -B $D/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/clang++ > $J/embed_$n.log 2>&1
nice cmake --build $D/build -j 4 >> $J/embed_$n.log 2>&1; rc=$?
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
(cd $D/build && ctest -j 4 --output-on-failure 2>&1 | tail -n 25) >> $J/embed_$n.log
st "embed test $n (build rc=$rc): $(grep 'tests passed' $J/embed_$n.log) $(grep -A3 'The following tests FAILED' $J/embed_$n.log | tr '\n' ' ') $($D/build/regicide bench 2>&1 | tail -n 1)"
cd ~/regicide-truth && git worktree remove --force $D
