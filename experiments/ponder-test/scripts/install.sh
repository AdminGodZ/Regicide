#!/usr/bin/env bash
# install.sh NAME: copy ~/regicide-ponder/build-win DLL to C:\regicide-match\engines\regicide\NAME with an egdb junction
set -euo pipefail
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:):/mnt/c/Windows/System32
NAME=$1; W=~/regicide-ponder
D=/mnt/c/regicide-match/engines/regicide/$NAME
mkdir -p $D
cp $W/build-win/libregicide_cb.dll $D/regicide_cb.dll
cd /mnt/c
if [ ! -e $D/egdb ]; then
  cmd.exe /c mklink /J 'C:\regicide-match\engines\regicide\'"$NAME"'\egdb' 'C:\regicide-match\engines\regicide\6bb8c6c-deep2b-egdb8\egdb' > /dev/null
fi
echo "egdb files: $(ls $D/egdb | wc -l)"
LLVM_MINGW=$(ls -d ~/regicide-match/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
{ echo "commit $(git -C $W/src rev-parse HEAD) (t3/ponder); book.bin = src/weights/book.bin sha256 $(sha256sum $W/src/src/weights/book.bin | cut -c1-16) (= book_deep2b, master)"
  echo "built $(date -Is) with $($LLVM_MINGW/bin/x86_64-w64-mingw32-clang++ --version | head -1)"
  echo "toolchain file: tools-src/tools/windows/mingw-clang.cmake; build tree ~/regicide-ponder/build-win"
  echo "egdb: junction to ..\6bb8c6c-deep2b-egdb8\egdb (2-8 pieces)"; } > $D/BUILD.txt
cat $D/BUILD.txt
