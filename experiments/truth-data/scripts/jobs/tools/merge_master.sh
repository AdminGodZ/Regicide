cd ~/regicide-truth
git fetch -q origin && git log --oneline -1 origin/master
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com merge --no-edit origin/master 2>&1 | tail -n 2
git diff --name-only --diff-filter=U
git log --oneline -3
sha256sum experiments/truth-data/n8q.nnue src/weights/net.bin
git show origin/master:experiments/truth-data/n8q.nnue | sha256sum
for i in 1 2 3; do git push -q origin t3/truth-data 2>&1 | tail -n 1; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 20; done
echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"
