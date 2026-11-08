#!/usr/bin/env bash
# NNUE round 5, step 1: build origin/t3/nnue-v2 in ~/regicide-v2 (Release, clang, native), ctest (12/12 expected),
# bench (7,338,658 expected, built-in n4c), random-net export checks v1 / v2 / v2 2x1024 L1 16.
set -u
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:) OMP_NUM_THREADS=1
git -C ~/regicide-nnue fetch -q origin
R=~/regicide-v2
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R origin/t3/nnue-v2
cd $R; echo "== $(git log --oneline -1)"; git status --short
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
grep -E "^REGICIDE_NATIVE|^CMAKE_BUILD_TYPE" build/CMakeCache.txt
nice cmake --build build -j 2 >> build.log 2>&1 || { tail -n 30 build.log; st "R5_BUILD_FAILED"; exit 1; }
echo "-- ctest"; nice ctest --test-dir build -j 2 --output-on-failure 2>&1 | tail -n 25
echo "-- bench"; ./build/regicide bench | tail -n 1
grep -o 'avx[a-z0-9_]*vnni[a-z0-9_]*' /proc/cpuinfo | sort -u | tr '\n' ' '; echo
for a in "--arch v1" "--arch v2" "--arch v2 --ft 1024 --l1 16"; do
  echo "-- random-net check_export $a"
  $PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-data/train/gen2b_n10k.bin $a 2>&1 | tail -n 4
done
st "R5_BUILD_DONE"
