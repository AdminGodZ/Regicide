#!/usr/bin/env bash
# commit_push.sh "MESSAGE" PATH...: commit the given paths in ~/regicide-truth and push t3/truth-data (with retries).
cd ~/regicide-truth
msg=$1; shift
git add -- "$@"
printf '%s\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n' "$msg" > /tmp/truth_msg.txt
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F /tmp/truth_msg.txt && git log --oneline -1
for i in 1 2 3 4 5; do git push -q origin t3/truth-data 2>&1 | tail -n 2; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 20; done
echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"
