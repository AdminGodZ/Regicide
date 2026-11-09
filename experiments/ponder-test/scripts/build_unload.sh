#!/usr/bin/env bash
set -e
LLVM_MINGW=$(ls -d ~/regicide-match/toolchain/llvm-mingw-*-ucrt-* | sort | tail -1)
$LLVM_MINGW/bin/x86_64-w64-mingw32-clang++ -O1 -std=c++20 -static -o /mnt/c/regicide-match/engines/regicide/ponder-df7780a-t/unload_check.exe /mnt/c/regicide-match/jobs/ponder/unload_check.cpp
echo built
