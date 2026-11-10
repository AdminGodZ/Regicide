cd ~/regicide-truth && git fetch -q origin
git log --oneline origin/master -6 -- tools/truth.cpp
echo ---; git log --oneline -6 -- tools/truth.cpp
echo ---; git diff 44cc6a4^2 44cc6a4 -- tools/truth.cpp | head -5
echo "--- master changes to truth.cpp not in HEAD:"
B=$(git merge-base HEAD origin/master); echo "merge-base $B"
for c in $(git log --format=%h origin/master -- tools/truth.cpp | head -6); do git merge-base --is-ancestor $c HEAD && echo "$c in HEAD" || echo "$c NOT in HEAD"; done
