#!/usr/bin/env bash
# commit.sh MSGFILE PATHS...   (commits in the booklab worktree)
set -e
cd ~/regicide-booklab
M=$1; shift
git add "$@"
git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F "$M"
git log --oneline -1
