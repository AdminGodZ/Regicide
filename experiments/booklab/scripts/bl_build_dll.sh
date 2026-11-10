#!/usr/bin/env bash
# Build a CheckerBoard DLL from the booklab worktree's HEAD with BOOK embedded as src/weights/book.bin.
#   bl_build_dll.sh NAME BOOK [JOBS]   ->  C:\regicide-match\engines\regicide\bl-NAME\regicide_cb.dll
# The engine's egdb/ folder is a directory junction to the 6bb8c6c-deep2b-egdb8 copy (2-8 pieces), not a new copy.
# Also builds the same tree for Linux and prints bench with the book on and off (bench must equal master's
# 7,712,020 / 8,089,571 unless the book changes a bench position).
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
NAME=$1; BOOK=$(readlink -f $2); JOBS=${3:-6}; REF=${4:-HEAD}   # REF: engine source (default the booklab HEAD)
RM=~/regicide-match; W=~/regicide-booklab
B=$RM/builds/bl-$NAME
if [ -d $B/src ]; then git -C $W worktree remove --force $B/src || rm -rf $B/src; fi
git -C $W worktree prune
mkdir -p $B; rm -rf $B/build $B/build-linux
git -C $W fetch -q origin
git -C $W worktree add --quiet --detach $B/src $REF
cp $BOOK $B/src/src/weights/book.bin
TC=$RM/tools-src/tools/windows/mingw-clang.cmake
export LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
cmake -S $B/src -B $B/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=$TC > $B/cmake.log 2>&1
nice cmake --build $B/build -j $JOBS --target regicide_cb >> $B/cmake.log 2>&1 || { tail -30 $B/cmake.log; exit 1; }
cmake -S $B/src -B $B/build-linux -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > $B/linux.log 2>&1
nice cmake --build $B/build-linux -j $JOBS --target regicide >> $B/linux.log 2>&1 || { tail -30 $B/linux.log; exit 1; }
python3 - $B/build/libregicide_cb.dll $BOOK <<'PY'
import sys
dll = open(sys.argv[1], 'rb').read(); b = open(sys.argv[2], 'rb').read()
print('book', len(b), 'bytes embedded at', dll.find(b))
assert dll.find(b) >= 0
PY
D=/mnt/c/regicide-match/engines/regicide/bl-$NAME
mkdir -p $D
cp $B/build/libregicide_cb.dll $D/regicide_cb.dll
if [ ! -e $D/egdb ]; then
  cmd.exe /c mklink /J "C:\\regicide-match\\engines\\regicide\\bl-$NAME\\egdb" "C:\\regicide-match\\engines\\regicide\\6bb8c6c-deep2b-egdb8\\egdb" > /dev/null
fi
ls $D/egdb | wc -l
{ echo "booklab: engine $(git -C $B/src rev-parse --short HEAD) ($REF); book.bin = $(basename $BOOK) sha256 $(sha256sum $BOOK | cut -c1-16)"
  echo "built $(date -Is) with $($LLVM_MINGW/bin/x86_64-w64-mingw32-clang++ --version | head -1)"
  echo "egdb: junction to ..\\6bb8c6c-deep2b-egdb8\\egdb (2-8 pieces)"; } > $D/BUILD.txt
cat $D/BUILD.txt
echo "-- linux bench (book on)"; $B/build-linux/regicide bench | tail -n 1
echo "-- linux bench (book off)"; printf 'set book off\nbench\nquit\n' | $B/build-linux/regicide 2>&1 | grep '^bench'
echo "installed $D"
