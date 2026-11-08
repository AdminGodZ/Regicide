#!/usr/bin/env bash
# Job 1: build + verify master 385611e (Linux: default FT 512 + width variants via CMake options;
# trainer worktrees with patched tools/nnue/nnue.py widths), then the Windows DLL (llvm-mingw).
set -u
J=~/regicide-jobs/r4; mkdir -p $J
st() { echo "$* $(date -Is)" >> $J/status; }
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
export OMP_NUM_THREADS=2
SLICE=~/regicide-nnue-work/slice_gen2b_200k.bin
SHA=385611e
R=~/regicide-385
git -C ~/regicide-nnue fetch -q origin
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R $SHA
echo "== $(git -C $R log --oneline -1)"; git -C $R status --short
cd $R
build() {  # build DIR [cmake opts...]
  local B=$1; shift
  cmake -S . -B $B -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ "$@" > $B.log 2>&1
  nice cmake --build $B -j ${JOBS:-4} >> $B.log 2>&1 || { tail -n 30 $B.log; return 1; }
  echo "-- $B ctest"; nice ctest --test-dir $B -j 4 2>&1 | tail -n 3
  echo "-- $B bench"; ./$B/regicide bench | tail -n 1
}
echo "== default (FT 512, L1 32, built-in nn2b)"
build build
cmp src/weights/net.bin ~/regicide-nnue2-work/nn2b.nnue && echo "built-in net.bin == nn2b.nnue"
echo "-- check_export (default trainer widths vs engine)"; $PY tools/nnue/check_export.py --engine build/regicide --data $SLICE 2>&1 | tail -n 3
for V in "ft768 -DREGICIDE_NNUE_FT=768" "ft1024 -DREGICIDE_NNUE_FT=1024" "ft768_l164 -DREGICIDE_NNUE_FT=768 -DREGICIDE_NNUE_L1=64"; do
  set -- $V; N=$1; shift
  echo "== $N ($*)"
  build build-$N "$@"
done
# trainer worktrees (tools/nnue/nnue.py widths only)
for V in "768 32 t768" "1024 32 t1024" "768 64 t768l64"; do
  set -- $V; T=~/regicide-385-$3
  [ -d $T ] || git -C ~/regicide-nnue worktree add -q --detach $T $SHA
  sed -i "s/^INPUTS, FT, L1, L2, BUCKETS = 128, 512, 32, 32, 8\$/INPUTS, FT, L1, L2, BUCKETS = 128, $1, $2, 32, 8/" $T/tools/nnue/nnue.py
  echo "== trainer $3: $(grep -n '^INPUTS, FT' $T/tools/nnue/nnue.py)"; git -C $T diff --stat
done
for V in "t768 build-ft768" "t1024 build-ft1024" "t768l64 build-ft768_l164"; do
  set -- $V
  echo "-- check_export random net, trainer $1 vs engine $2"
  (cd ~/regicide-385-$1 && $PY tools/nnue/check_export.py --engine $R/$2/regicide --data $SLICE 2>&1 | tail -n 3)
done
st "R4_LINUX_BUILD_DONE"
echo "== Windows DLL"
SHA=$SHA OUT=$SHA bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh
st "R4_BUILD_DONE"
