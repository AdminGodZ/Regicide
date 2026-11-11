#!/usr/bin/env bash
# Windows CheckerBoard DLL of this branch (llvm-mingw, as jobs/ponder/build.sh) -> engines/regicide/truth-<sha>.
# Stops on any failed step: the old DLL and exe are deleted before the build, and the copied DLL must be newer than
# the start of this script. Also builds regicide.exe from the same tree and records its bench in BUILD.txt (the DLL
# itself has no bench command).
source /mnt/c/regicide-match/jobs/truth/common.sh
set -eo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
RM=~/regicide-match; S=~/regicide-truth; B=$S/build-win
TC=$RM/tools-src/tools/windows/mingw-clang.cmake
export LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
t0=$(date +%s)
touch $LOCKS/yield_truth
trap 'pgrep -f "jobs/truth/run_cb.sh" > /dev/null || rm -f $LOCKS/yield_truth' EXIT
rm -f $B/libregicide_cb.dll $B/regicide.exe
if [ ! -f $B/build.ninja ]; then
  cmake -S $S -B $B -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TC > $J/build_win.log 2>&1 \
    || { st "windows build: configure FAILED (see $J/build_win.log)"; exit 1; }
fi
nice -n 15 cmake --build $B -j 4 --target regicide_cb regicide >> $J/build_win.log 2>&1 \
  || { st "windows build: build FAILED (see $J/build_win.log)"; exit 1; }
[ -f $B/libregicide_cb.dll ] && [ $(stat -c %Y $B/libregicide_cb.dll) -ge $t0 ] \
  || { st "windows build: no fresh DLL"; exit 1; }
sha=$(git -C $S log -1 --format=%h -- src CMakeLists.txt); D=/mnt/c/regicide-match/engines/regicide/truth-$sha  # the last commit that changed the engine
[ -z "$(git -C $S status --porcelain -- src CMakeLists.txt)" ] || { st "windows build: src has uncommitted changes"; exit 1; }
mkdir -p $D; cp $B/libregicide_cb.dll $D/regicide_cb.dll; cp $B/regicide.exe $D/regicide.exe
[ $(stat -c %Y $D/regicide_cb.dll) -ge $t0 ] && cmp -s $B/libregicide_cb.dll $D/regicide_cb.dll \
  || { st "windows build: copy check FAILED"; exit 1; }
bench=$(cd $D && ./regicide.exe bench 2>&1 | tr -d '\r' | tail -n 1)
echo "truth-data: engine $sha (t3/truth-data); built $(date -Is) with $(basename $LLVM_MINGW); regicide.exe from the same tree: $bench" > $D/BUILD.txt
st "windows dll built: $D ($bench)"
echo $D
