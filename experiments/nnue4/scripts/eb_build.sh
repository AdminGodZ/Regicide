#!/usr/bin/env bash
# Build + verify master eb1037c (default FT 768, built-in nn3e): Linux ctest + bench, Windows DLL.
set -u
J=~/regicide-jobs/r4
st() { echo "$* $(date -Is)" >> $J/status; }
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
SHA=eb1037c; R=~/regicide-eb1
git -C ~/regicide-nnue fetch -q origin
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R $SHA
cd $R; echo "== $(git log --oneline -1)"; git status --short
cmp src/weights/net.bin ~/regicide-nnue3-work/nn3e.nnue && echo "built-in net.bin == nn3e.nnue"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
nice cmake --build build -j 4 >> build.log 2>&1 || tail -n 30 build.log
echo "-- ctest"; nice ctest --test-dir build -j 4 2>&1 | tail -n 3
echo "-- bench"; ./build/regicide bench | tail -n 1
echo "-- bench of 385611e build-ft768 with set evalfile nn3e (should match)"
printf "set evalfile $HOME/regicide-nnue3-work/nn3e.nnue\nbench\nquit\n" | ~/regicide-385/build-ft768/regicide | grep '^bench'
st "EB_LINUX_DONE"
echo "== Windows DLL"
SHA=$SHA OUT=$SHA bash /mnt/c/regicide-match/jobs/r4/build_win_variant.sh
st "EB_BUILD_DONE"
