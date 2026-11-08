#!/usr/bin/env bash
# Windows build (llvm-mingw clang, Release) of a44591e: regicide_cb DLL + regicide.exe; install DLL with an egdb copy.
set -euo pipefail
SHA_REQ=a44591e
RM=~/regicide-match
CACHE=$RM/cache/regicide
git -C "$CACHE" fetch --quiet --prune origin '+refs/heads/*:refs/remotes/origin/*'
SHA=$(git -C "$CACHE" rev-parse "$SHA_REQ^{commit}")
SHORT=${SHA:0:7}
BUILD=$RM/builds/$SHORT
if [ -d "$BUILD/src" ]; then git -C "$CACHE" worktree remove --force "$BUILD/src" || rm -rf "$BUILD/src"; fi
git -C "$CACHE" worktree prune
mkdir -p "$BUILD"
git -C "$CACHE" worktree add --quiet --detach "$BUILD/src" "$SHA"
TC=$RM/tools-src/tools/windows/mingw-clang.cmake
LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
export LLVM_MINGW
cmake -S "$BUILD/src" -B "$BUILD/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$TC" > "$BUILD/cmake.log" 2>&1
cmake --build "$BUILD/build" -j 8 >> "$BUILD/cmake.log" 2>&1 || { tail -40 "$BUILD/cmake.log"; exit 1; }
ls -la "$BUILD/build"/*.dll "$BUILD/build"/*.exe
# embedded data check: eval.bin and book.bin bytes must be inside the DLL
python3 - "$BUILD/build/libregicide_cb.dll" "$BUILD/src/src/weights/eval.bin" "$BUILD/src/src/weights/book.bin" <<'PY'
import sys
dll=open(sys.argv[1],'rb').read()
for f in sys.argv[2:]:
    b=open(f,'rb').read(); print(f, len(b), 'embedded at', dll.find(b))
PY
DLL_DIR=/mnt/c/regicide-match/engines/regicide/$SHORT
mkdir -p "$DLL_DIR"
cp "$BUILD/build/libregicide_cb.dll" "$DLL_DIR/regicide_cb.dll"
if [ ! -d "$DLL_DIR/egdb" ]; then cp -r ~/regicide-data/egdb "$DLL_DIR/egdb"; fi
diff -rq ~/regicide-data/egdb "$DLL_DIR/egdb" && echo "egdb copy identical"
W=/mnt/c/regicide-match/wintest/$SHORT
mkdir -p $W
cp "$BUILD/build/regicide.exe" "$BUILD/build/libregicide_cb.dll" "$BUILD/build/cb_api_test.exe" "$BUILD/build/perft_test.exe" $W/
{ echo "commit $SHA (master)"; echo "no patches";
  echo "built $(date -Is) with $($LLVM_MINGW/bin/x86_64-w64-mingw32-clang++ --version | head -1)";
  echo "toolchain file: tools-src/tools/windows/mingw-clang.cmake (same as b71dfa9 build)";
  echo "embedded: src/weights/eval.bin (= experiments/gen1full/wd/w_lr0.03_wd0.bin), src/weights/book.bin (= book_b13.bin)";
  echo "egdb: full copy of ~/regicide-data/egdb (2-7 pieces) in egdb/"; } > "$DLL_DIR/BUILD.txt"
echo "installed $DLL_DIR"
