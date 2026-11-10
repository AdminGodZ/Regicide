set -e
cd ~/regicide-truth
cp /mnt/c/regicide-match/jobs/truth/tools/truth.cpp tools/truth.cpp
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
nice cmake --build build -j 3 --target truth 2>&1 | grep -E " error" | head || true
printf '[Event "a"]\n[Black "X"]\n[White "Regicide"]\n[Result "1-0"]\n[FEN "W:WK22:B9,10,17,18,25"]\n1. 22x29 1-0\n\n[Event "b"]\n[Black "Regicide"]\n[White "X"]\n[Result "0-1"]\n[FEN "B:W6,7,14,15,32:B2"]\n1. 2x18 0-1\n' > /tmp/amb2.pdn
./build/truth losswalk --egdb ~/regicide-data/egdb --nodes 1000 --threads 1 --out /tmp/amb2.tsv /tmp/amb2.pdn > /dev/null; cut -f2,3,8,9,10 /tmp/amb2.tsv
git fetch -q origin
git add tools/truth.cpp
printf 'truth losswalk: ambiguity check follows the parsers identity rule (a legal single jump is that jump; captures are told apart by the pieces they take)\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n' > /tmp/m.txt
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F /tmp/m.txt
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com merge --no-edit origin/master 2>&1 | tail -n 1
git diff --name-only --diff-filter=U
cmake -S . -B build > /dev/null; nice cmake --build build -j 4 2>&1 | grep -E " error" | head || true
(cd build && ctest -j 4 2>&1 | grep -E "tests passed|FAILED")
./build/regicide bench 2>&1 | tail -n 1
for f in truth.cpp datagen.cpp; do cp tools/$f /mnt/c/regicide-match/jobs/truth/tools/$f; done; cp tools/nnue/nnue.py /mnt/c/regicide-match/jobs/truth/tools/nnue.py
for i in 1 2 3; do git push -q origin t3/truth-data 2>&1 | tail -n 1; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 20; done
git log --oneline -2; echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"
