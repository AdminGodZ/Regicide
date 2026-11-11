#!/usr/bin/env bash
# Runner (Linux match) and host (Windows cbhost.exe) from one master commit, for timed booklab runs.
#   build_runner.sh REF TAG   ->  ~/regicide-jobs/booklab/bin/match-TAG, bin/TAG/cbhost.exe (-> C:\regicide-match\bin\REF)
# Nice'd, 2 build threads. Prints the sha256 prefixes and the runner's options of interest.
set -e
REF=$1; TAG=$2
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
RM=~/regicide-match; W=~/regicide-booklab; B=$RM/builds/$TAG; L=~/regicide-jobs/booklab
[ -d $B/src ] && { git -C $W worktree remove --force $B/src || rm -rf $B/src; }
git -C $W fetch -q origin; git -C $W worktree prune; mkdir -p $B
git -C $W worktree add --quiet --detach $B/src $REF
FULL=$(git -C $B/src rev-parse HEAD)
cmake -S $B/src -B $B/build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $B/linux.log 2>&1
nice cmake --build $B/build-linux -j 2 --target match >> $B/linux.log 2>&1
export LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
cmake -S $B/src -B $B/build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$W/tools/windows/mingw-clang.cmake > $B/win.log 2>&1
nice cmake --build $B/build-win -j 2 --target cbhost >> $B/win.log 2>&1
mkdir -p /mnt/c/regicide-match/bin/${FULL:0:7} $L/bin/$TAG
cp $B/build-linux/tools/match $L/bin/match-$TAG
cp $B/build-win/tools/cbhost.exe /mnt/c/regicide-match/bin/${FULL:0:7}/cbhost.exe
ln -sf /mnt/c/regicide-match/bin/${FULL:0:7}/cbhost.exe $L/bin/$TAG/cbhost.exe
echo "runner $TAG from $FULL"
sha256sum $L/bin/match-$TAG /mnt/c/regicide-match/bin/${FULL:0:7}/cbhost.exe | cut -c1-16
$L/bin/match-$TAG --help 2>&1 | grep -i -A1 "fresh-engines\|pairs-together" || true
