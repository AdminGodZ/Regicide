cd ~/regicide-truth
git status --short | head -3
git fetch -q origin && git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com merge --no-edit origin/master 2>&1 | tail -n 2
git diff --name-only --diff-filter=U
sha256sum src/weights/net.bin | cut -c1-16
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd:)
nice cmake --build build -j 4 2>&1 | grep -E " error|warning: " | grep -v store.cpp | head
(cd build && ctest -j 4 2>&1 | grep -E "tests passed|FAILED")
./build/regicide bench 2>&1 | tail -n 1
printf 'set book off\nbench\nquit\n' | ./build/regicide 2>&1 | grep -o "bench: .*"
for i in 1 2 3; do git push -q origin t3/truth-data 2>&1 | tail -n 1; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 20; done
git log --oneline -1; echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"
