#!/usr/bin/env bash
# Windows CheckerBoard DLL of this branch (llvm-mingw, as jobs/ponder/build.sh) -> engines/regicide/truth-<sha>.
source /mnt/c/regicide-match/jobs/truth/common.sh
set -o pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
RM=~/regicide-match; S=~/regicide-truth; B=$S/build-win
TC=$RM/tools-src/tools/windows/mingw-clang.cmake
export LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
touch $LOCKS/yield_truth
[ -f $B/build.ninja ] || cmake -S $S -B $B -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TC > $J/build_win.log 2>&1
nice -n 15 cmake --build $B -j 4 --target regicide_cb >> $J/build_win.log 2>&1
pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth
sha=$(git -C $S rev-parse --short HEAD); D=/mnt/c/regicide-match/engines/regicide/truth-$sha
mkdir -p $D; cp $B/libregicide_cb.dll $D/regicide_cb.dll
echo "truth-data: engine $sha (t3/truth-data, master 2efe11a merged); built $(date -Is) with $(ls -d $LLVM_MINGW | xargs basename)" > $D/BUILD.txt
st "windows dll built: $D"
echo $D
