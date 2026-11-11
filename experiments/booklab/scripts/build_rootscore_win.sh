# booklab_rootscore for Windows (mingw-clang, as the engine DLLs), from the scorer worktree of 1c6e997, so that the
# WP14 searches run as Windows processes pinned to cores 0-19 (start /affinity), away from the (b) runs' cores 21-23.
set -e
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
RM=~/regicide-match; W=~/regicide-booklab; B=$RM/builds/scorer-1c6e
export LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
cmake -S $B/src -B $B/build-win -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$W/tools/windows/mingw-clang.cmake > $B/win.log 2>&1
nice cmake --build $B/build-win -j 2 --target booklab_rootscore >> $B/win.log 2>&1 || { tail -20 $B/win.log; exit 1; }
mkdir -p /mnt/c/regicide-match/bin/wp14
cp $B/build-win/booklab_rootscore.exe /mnt/c/regicide-match/bin/wp14/
for f in $B/build-win/*.dll; do [ -f "$f" ] && cp "$f" /mnt/c/regicide-match/bin/wp14/; done
ls -la /mnt/c/regicide-match/bin/wp14/ | tail -4; sha256sum /mnt/c/regicide-match/bin/wp14/booklab_rootscore.exe | cut -c1-16
