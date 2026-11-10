set -e
cd ~/regicide-truth
git add tools/truth.cpp tests/truth_fens_test.cmake CMakeLists.txt
printf 'truth fens: restore the no-complete-record guard (PR #41) lost in 915dcfb, check the input before the database loads, and add ctest truth_fens_no_record (empty and 15-byte file)\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n' > /tmp/m.txt
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F /tmp/m.txt && git log --oneline -1
git fetch -q origin
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com merge --no-edit origin/master 2>&1 | tail -n 1
git diff --name-only --diff-filter=U
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
cmake -S . -B build > /dev/null; nice cmake --build build -j 4 2>&1 | grep -E " error" | head
(cd build && ctest -j 4 2>&1 | grep -E "tests passed|FAILED")
./build/regicide bench 2>&1 | tail -n 1
echo "=== master lines missing from HEAD after the merge (should be none):"
for f in tools/truth.cpp tools/datagen.cpp tools/rescore.cpp tools/nnue/nnue.py tools/nnue/train.py CMakeLists.txt; do git diff HEAD origin/master -- $f | grep -cE "^\+[^+]" | sed "s|^|$f: |"; done
for f in truth.cpp datagen.cpp; do cp tools/$f /mnt/c/regicide-match/jobs/truth/tools/$f; done
cp tools/nnue/nnue.py /mnt/c/regicide-match/jobs/truth/tools/nnue.py
for i in 1 2 3; do git push -q origin t3/truth-data 2>&1 | tail -n 1; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 20; done
git log --oneline -1; echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"
