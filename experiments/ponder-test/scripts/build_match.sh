#!/usr/bin/env bash
set -euo pipefail
W=~/regicide-ponder
[ -f $W/build-tools/build.ninja ] || cmake -S $W/src/tools -B $W/build-tools -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $W/tools.log 2>&1 || { tail -20 $W/tools.log; exit 1; }
nice -n 15 cmake --build $W/build-tools -j 4 --target match >> $W/tools.log 2>&1 || { tail -30 $W/tools.log; exit 1; }
SHA=$(git -C $W/src rev-parse --short HEAD)
cp $W/build-tools/match ~/regicide-match/bin/match-ponder-$SHA
ls -la ~/regicide-match/bin/
~/regicide-match/bin/match-ponder-$SHA --help 2>&1 | head -40
