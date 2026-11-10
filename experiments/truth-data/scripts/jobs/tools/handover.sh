#!/usr/bin/env bash
# Hand-over: n8q as the built-in net (src/weights/net.bin) on t3/truth-data; build, ctest, bench.
source /mnt/c/regicide-match/jobs/truth/common.sh
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
cd ~/regicide-truth
git status --short | head
cp ~/regicide-truth-work/n8q.nnue src/weights/net.bin
cmp src/weights/net.bin experiments/truth-data/n8q.nnue && echo "net.bin == experiments/truth-data/n8q.nnue ($(sha256sum src/weights/net.bin | cut -c1-16))"
nice cmake --build build -j 4 2>&1 | grep -E "error|warning: " | grep -v store.cpp | head
(cd build && ctest -j 4 2>&1 | grep -E "tests passed|FAILED|Failed")
./build/regicide bench 2>&1 | tail -n 1
printf 'set book off\nbench\nquit\n' | ./build/regicide 2>&1 | grep -o "bench: .*"
