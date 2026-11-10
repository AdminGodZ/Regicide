set -e
cd ~/regicide-truth
cp /mnt/c/regicide-match/jobs/truth/tools/truth.cpp tools/truth.cpp
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
nice cmake --build build -j 3 --target truth 2>&1 | grep -E " error" | head || true
./build/truth losswalk --egdb ~/regicide-data/egdb --nodes 1000 --threads 1 --out /tmp/amb2.tsv /tmp/amb2.pdn > /dev/null; cut -f2,3,8 /tmp/amb2.tsv
printf '[Event "c"]\n[Black "Regicide"]\n[White "X"]\n[Result "0-1"]\n[FEN "B:W6,7,14,15,32:B2"]\n1. 2x9x18 0-1\n' > /tmp/amb3.pdn
./build/truth losswalk --egdb ~/regicide-data/egdb --nodes 1000 --threads 1 --out /tmp/amb3.tsv /tmp/amb3.pdn > /dev/null; cut -f2,3,8 /tmp/amb3.tsv | tail -n 1
(cd build && ctest -j 4 2>&1 | grep -E "tests passed|FAILED")
git add tools/truth.cpp
printf 'truth losswalk: a single jump is a capture of one piece (move_to_string prints end squares only)\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n' > /tmp/m.txt
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F /tmp/m.txt
for i in 1 2 3; do git push -q origin t3/truth-data 2>&1 | tail -n 1; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 20; done
git log --oneline -1; echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"
