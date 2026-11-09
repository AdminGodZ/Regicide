#!/usr/bin/env bash
# build.sh [REF]: build t3/ponder (or REF) for Windows (DLL + ponder_test.exe + cb_api_test.exe, llvm-mingw) and Linux
# (regicide, match, tests). Tree ~/regicide-ponder (clone of the Windows checkout). Logs in ~/regicide-ponder/*.log.
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
REF=${1:-win/t3/ponder}; JOBS=${JOBS:-4}
W=~/regicide-ponder; RM=~/regicide-match
[ -d $W/src ] || git clone -q /mnt/c/Users/micro/Documents/AdminGod/Regicide $W/src
git -C $W/src fetch -q origin '+refs/remotes/origin/*:refs/remotes/win/*' '+refs/heads/*:refs/remotes/winlocal/*'
git -C $W/src checkout -q --detach $REF
git -C $W/src log --oneline -1
TC=$RM/tools-src/tools/windows/mingw-clang.cmake
export LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
[ -f $W/build-win/build.ninja ] || cmake -S $W/src -B $W/build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TC > $W/win.log 2>&1
nice -n 15 cmake --build $W/build-win -j $JOBS --target regicide_cb ponder_test cb_api_test > $W/win.log 2>&1 || { grep -E "error|warning" $W/win.log | head -40; tail -20 $W/win.log; exit 1; }
grep -c warning $W/win.log || true
[ -f $W/build-linux/build.ninja ] || cmake -S $W/src -B $W/build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $W/linux.log 2>&1
nice -n 15 cmake --build $W/build-linux -j $JOBS > $W/linux.log 2>&1 || { tail -30 $W/linux.log; exit 1; }
echo "built OK"
