#!/usr/bin/env bash
# Windows (llvm-mingw, Release) variant build of SHA with optional replacements, installed as engines/regicide/$OUT
# with a 2-7 piece egdb copy. Env: SHA, OUT, NETFILE (-> src/weights/net.bin), PATCH (git apply), BOOKFILE
# (-> src/weights/book.bin). Also builds the same tree for Linux (for checks) in $BUILD/build-linux.
set -euo pipefail
SHA_REQ=${SHA:?}; OUT=${OUT:?}
RM=~/regicide-match
CACHE=$RM/cache/regicide
git -C "$CACHE" fetch --quiet --prune origin '+refs/heads/*:refs/remotes/origin/*'
FULL=$(git -C "$CACHE" rev-parse "$SHA_REQ^{commit}")
BUILD=$RM/builds/$OUT
if [ -d "$BUILD/src" ]; then git -C "$CACHE" worktree remove --force "$BUILD/src" || rm -rf "$BUILD/src"; fi
git -C "$CACHE" worktree prune
mkdir -p "$BUILD"; rm -rf "$BUILD/build" "$BUILD/build-linux"
git -C "$CACHE" worktree add --quiet --detach "$BUILD/src" "$FULL"
NOTES="commit $FULL${CMAKE_EXTRA:+; cmake $CMAKE_EXTRA}"
[ -n "${PATCH:-}" ] && { git -C "$BUILD/src" apply "$PATCH"; NOTES="$NOTES; patch $(basename $PATCH)"; }
[ -n "${NETFILE:-}" ] && { cp "$NETFILE" "$BUILD/src/src/weights/net.bin"; NOTES="$NOTES; net.bin = $(basename $NETFILE) sha256 $(sha256sum $NETFILE | cut -c1-16)"; }
[ -n "${BOOKFILE:-}" ] && { cp "$BOOKFILE" "$BUILD/src/src/weights/book.bin"; NOTES="$NOTES; book.bin = $(basename $BOOKFILE) sha256 $(sha256sum $BOOKFILE | cut -c1-16)"; }
git -C "$BUILD/src" status --short
TC=$RM/tools-src/tools/windows/mingw-clang.cmake
LLVM_MINGW=$(ls -d $RM/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
export LLVM_MINGW
cmake -S "$BUILD/src" -B "$BUILD/build" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE="$TC" ${CMAKE_EXTRA:-} > "$BUILD/cmake.log" 2>&1
nice cmake --build "$BUILD/build" -j ${JOBS:-4} >> "$BUILD/cmake.log" 2>&1 || { tail -40 "$BUILD/cmake.log"; exit 1; }
cmake -S "$BUILD/src" -B "$BUILD/build-linux" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ ${CMAKE_EXTRA:-} > "$BUILD/linux.log" 2>&1
nice cmake --build "$BUILD/build-linux" -j ${JOBS:-4} >> "$BUILD/linux.log" 2>&1 || { tail -40 "$BUILD/linux.log"; exit 1; }
python3 - "$BUILD/build/libregicide_cb.dll" "$BUILD/src/src/weights/eval.bin" "$BUILD/src/src/weights/net.bin" "$BUILD/src/src/weights/book.bin" <<'PY'
import sys, os
dll = open(sys.argv[1], 'rb').read()
for f in sys.argv[2:]:
    b = open(f, 'rb').read()
    print(os.path.basename(f), len(b), 'embedded at', dll.find(b) if b else '(empty)')
PY
DLL_DIR=/mnt/c/regicide-match/engines/regicide/$OUT
mkdir -p "$DLL_DIR"
cp "$BUILD/build/libregicide_cb.dll" "$DLL_DIR/regicide_cb.dll"
[ -d "$DLL_DIR/egdb" ] || cp -r ~/regicide-data/egdb "$DLL_DIR/egdb"
diff -rq ~/regicide-data/egdb "$DLL_DIR/egdb" && echo "egdb copy identical"
WT=/mnt/c/regicide-match/wintest/$OUT
mkdir -p $WT
cp "$BUILD/build/regicide.exe" "$BUILD/build/libregicide_cb.dll" "$BUILD/build/perft_test.exe" $WT/
{ echo "$NOTES"; echo "built $(date -Is) with $($LLVM_MINGW/bin/x86_64-w64-mingw32-clang++ --version | head -1)";
  echo "toolchain file: tools-src/tools/windows/mingw-clang.cmake"; echo "egdb: full copy of ~/regicide-data/egdb (2-7 pieces) in egdb/"; } > "$DLL_DIR/BUILD.txt"
cat "$DLL_DIR/BUILD.txt"
echo "-- regicide.exe bench (Windows)"; (cd $WT && ./regicide.exe bench | tail -n 1)
echo "-- linux bench"; "$BUILD/build-linux/regicide" bench | tail -n 1
echo "installed $DLL_DIR"
