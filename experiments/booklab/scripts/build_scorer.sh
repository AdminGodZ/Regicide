#!/usr/bin/env bash
# booklab tools (multiscore, override, pinwalk) on an engine commit, e.g. master 1c6e997 (n8q, r11d): a detached
# worktree of REF plus tools/booklab/*.cpp of the booklab branch and their CMake targets; nothing of the engine changes.
#   build_scorer.sh REF TAG  ->  ~/regicide-jobs/booklab/bin/blk-multiscore-TAG, blk-override-TAG, blk-pinwalk-TAG
set -e
REF=$1; TAG=$2
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
RM=~/regicide-match; W=~/regicide-booklab; B=$RM/builds/scorer-$TAG; L=~/regicide-jobs/booklab
[ -d $B/src ] && { git -C $W worktree remove --force $B/src || rm -rf $B/src; }
git -C $W fetch -q origin; git -C $W worktree prune; mkdir -p $B
git -C $W worktree add --quiet --detach $B/src $REF
mkdir -p $B/src/tools/booklab
cp $W/tools/booklab/*.cpp $B/src/tools/booklab/
for t in multiscore override pinwalk; do
  printf '\nadd_executable(booklab_%s tools/booklab/%s.cpp)\ntarget_link_libraries(booklab_%s PRIVATE regicide_core)\n' $t $t $t >> $B/src/CMakeLists.txt
done
cmake -S $B/src -B $B/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $B/cmake.log 2>&1
nice cmake --build $B/build -j 2 --target booklab_multiscore booklab_override booklab_pinwalk >> $B/cmake.log 2>&1 || { tail -20 $B/cmake.log; exit 1; }
for t in multiscore override pinwalk; do cp $B/build/booklab_$t $L/bin/blk-$t-$TAG; done
echo "engine $(git -C $B/src rev-parse --short HEAD); net $(sha256sum $B/src/src/weights/net.bin | cut -c1-16); book $(sha256sum $B/src/src/weights/book.bin | cut -c1-16)"
echo "engine source changes against REF (only CMakeLists.txt and the added tools/booklab may differ):"; git -C $B/src status --short | head
sha256sum $L/bin/blk-multiscore-$TAG | cut -c1-16
