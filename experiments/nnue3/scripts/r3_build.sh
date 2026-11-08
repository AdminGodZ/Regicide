#!/usr/bin/env bash
# Step 1: build and verify master 825f993 (Linux), plus scratch worktrees FT 1024 and L1 64 (empty net.bin).
set -u
J=~/regicide-jobs/r3; mkdir -p $J
st() { echo "$* $(date -Is)" >> $J/status; }
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
export OMP_NUM_THREADS=2
SLICE=~/regicide-nnue-work/slice_gen2b_200k.bin
git -C ~/regicide-nnue fetch -q origin
build() {  # build DIR
  cd $1
  cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
  nice cmake --build build -j 10 >> build.log 2>&1 || { tail -n 30 build.log; return 1; }
  echo "-- ctest"; nice ctest --test-dir build -j 6 2>&1 | tail -n 4
  echo "-- bench"; ./build/regicide bench | tail -n 1
  echo "-- check_export (random net)"; $PY tools/nnue/check_export.py --engine build/regicide --data $SLICE 2>&1 | tail -n 4
}
R=~/regicide-825
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R 825f993
echo "== 825f993 $(git -C $R log --oneline -1)"
build $R
for V in ft1024 l1_64; do
  W=~/regicide-825-$V
  [ -d $W ] || git -C ~/regicide-nnue worktree add -q --detach $W 825f993
  cd $W
  if [ $V = ft1024 ]; then
    sed -i 's/^constexpr int FT = 512;/constexpr int FT = 1024;/' src/nnue.hpp
    sed -i 's/^INPUTS, FT, L1, L2, BUCKETS = 128, 512, 32, 32, 8$/INPUTS, FT, L1, L2, BUCKETS = 128, 1024, 32, 32, 8/' tools/nnue/nnue.py
  else
    sed -i 's/^constexpr int L1 = 32;/constexpr int L1 = 64;/' src/nnue.hpp
    sed -i 's/^INPUTS, FT, L1, L2, BUCKETS = 128, 512, 32, 32, 8$/INPUTS, FT, L1, L2, BUCKETS = 128, 512, 64, 32, 8/' tools/nnue/nnue.py
  fi
  : > src/weights/net.bin   # the embedded nn2b does not fit this architecture: linear fallback unless set evalfile
  echo "== $V"; git diff --stat; grep -n '^constexpr int \(FT\|L1\) ' src/nnue.hpp; grep -n '^INPUTS, FT' tools/nnue/nnue.py
  build $W
  echo "-- nn2b rejected: $(printf 'set evalfile %s\nquit\n' ~/regicide-nnue2-work/nn2b.nnue | ./build/regicide 2>&1 | grep -i -m1 'cannot\|expects')"
done
st "R3_BUILD_DONE"
