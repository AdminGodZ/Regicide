#!/usr/bin/env bash
# P5: a build of this branch that loads nets up to FT 2048 (REGICIDE_NNUE_MAX_FT=2048): regicide (evaldata, bench) and
# libregicide_cb.so for the wide-net matches (both sides use this one .so).
source /mnt/c/regicide-match/jobs/truth/common.sh
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
touch $LOCKS/yield_truth
cd ~/regicide-truth
[ -d build-ft2048 ] || cmake -S . -B build-ft2048 -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=/usr/bin/clang++ -DREGICIDE_NNUE_MAX_FT=2048 > /dev/null
nice cmake --build build-ft2048 -j 4 --target regicide regicide_cb > $J/build_ft2048.log 2>&1
rc=$?
pgrep -f "jobs/truth/run_cb.sh|jobs/truth/group_loss.sh" > /dev/null || rm -f $LOCKS/yield_truth
mkdir -p ~/regicide-truth-work/engines
cp build-ft2048/libregicide_cb.so ~/regicide-truth-work/engines/rft2048.so
st "ft2048 build rc=$rc at $(git rev-parse --short HEAD): $(./build-ft2048/regicide bench 2>&1 | tail -n 1)"
