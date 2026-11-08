#!/usr/bin/env bash
# Build tools/bookgen (and regicide, for the bench check) from master cc9f35f; copy bookgen to ~/regicide-jobs/book/bin.
set -u
JB=~/regicide-jobs/book; mkdir -p $JB/bin
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
SHA=cc9f35f; R=~/regicide-cc9
git -C ~/regicide-nnue fetch -q origin
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R $SHA
cd $R; echo "== $(git log --oneline -1)"; git status --short
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
nice cmake --build build -j 3 --target bookgen regicide >> build.log 2>&1 || { tail -n 30 build.log; exit 1; }
echo "-- bench"; ./build/regicide bench | tail -n 1
grep -E "se_depth|se_margin" src/search.hpp | head -4
cp build/bookgen $JB/bin/bookgen-$SHA; sha256sum $JB/bin/bookgen-$SHA
echo "BOOK_BUILD_DONE $(date -Is)" >> $JB/status
