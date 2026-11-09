#!/usr/bin/env bash
# Linux build of master ff0a3d2 (built-in n6c): libregicide_cb.so, tools/match, cbhost, bookgen; ctest + bench
# (8,089,571 expected). Used for the round-6b matches (vs the built-in n6c) and the deep book's bookgen.
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
SHA=ff0a3d2; R=~/regicide-ff0; W=~/regicide-nnue6-work; JD=~/regicide-jobs/deep
git -C ~/regicide-nnue fetch -q origin
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R $SHA
cd $R; echo "== $(git log --oneline -1)"
cmp src/weights/net.bin $W/n6c.nnue && echo "built-in net.bin == n6c.nnue"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
nice cmake --build build -j 3 >> build.log 2>&1 || { tail -n 30 build.log; exit 1; }
echo "-- ctest"; nice ctest --test-dir build -j 3 2>&1 | tail -n 3
echo "-- bench"; ./build/regicide bench | tail -n 1
mkdir -p $W/engines $W/bin $JD/bin
cp build/libregicide_cb.so $W/engines/rff0a3d2.so
[ -e $W/engines/egdb ] || ln -s ~/regicide-data/egdb $W/engines/egdb
cp build/tools/match $W/bin/match-ff0a3d2; cp build/tools/cbhost $W/bin/cbhost
cp build/bookgen $JD/bin/bookgen-ff0a3d2
sha256sum $W/engines/rff0a3d2.so $JD/bin/bookgen-ff0a3d2
echo "FF0_BUILD_DONE $(date -Is)" >> ~/regicide-jobs/r4/status
