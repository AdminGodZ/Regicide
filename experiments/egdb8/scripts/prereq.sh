#!/usr/bin/env bash
# egdb8 steps 3-4: build egdb_gen/egdb_test from master (SHA env, default origin/master; Release,
# REGICIDE_NATIVE=ON -> -march=native), copy to ~/regicide-jobs/egdb8/bin, check prerequisites on
# ~/regicide-data/egdb (read-only checks).
set -u
JE=~/regicide-jobs/egdb8; mkdir -p $JE/bin
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
git -C ~/regicide-nnue fetch -q origin
SHA=$(git -C ~/regicide-nnue rev-parse --short ${SHA:-origin/master})
R=~/regicide-egdb8-src
[ -d $R ] || git -C ~/regicide-nnue worktree add -q --detach $R $SHA
cd $R; echo "== $(git log --oneline -1)"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ > build.log 2>&1
grep -E "^REGICIDE_NATIVE|^CMAKE_BUILD_TYPE" build/CMakeCache.txt
nice cmake --build build -j 4 --target egdb_gen egdb_test >> build.log 2>&1 || { tail -n 30 build.log; exit 1; }
grep -c march=native build/compile_commands.json | sed 's/^/compile commands with -march=native: /'
cp build/egdb_gen $JE/bin/egdb_gen; cp build/egdb_test $JE/bin/egdb_test; echo $SHA > $JE/bin/SHA
sha256sum $JE/bin/egdb_gen $JE/bin/egdb_test
echo "== usage"; $JE/bin/egdb_gen 2>&1 | head -40
D=~/regicide-data/egdb
echo "== manifest"; $JE/bin/egdb_gen manifest --dir $D > $JE/manifest_before.txt 2>&1; echo "manifest rc=$?"; tail -n 5 $JE/manifest_before.txt
echo "== egdb_test --data"; REGICIDE_EGDB=$D $JE/bin/egdb_test --data > $JE/egdb_test_data_before.txt 2>&1; echo "egdb_test --data rc=$?"; tail -n 5 $JE/egdb_test_data_before.txt
echo "== disk"; df -h ~ | tail -n 1; df -B1G --output=avail ~ | tail -n 1 | sed 's/^/avail GB: /'
echo "== ram"; free -g
echo "== db dir"; du -sh $D; ls $D | head; ls $D | wc -l; ls -d $D/parts 2>/dev/null
