cd ~/regicide-truth
git ls-tree --name-only origin/t3/booklab experiments/booklab/wp10/games/ | head; git ls-tree --name-only origin/t3/booklab experiments/booklab/wp10/runs/ | head
git show origin/t3/booklab:experiments/booklab/wp10/games.tsv | head -3
ls -d ~/regicide-match/results/bl-f4-cake-h1 ~/regicide-match/results/bl-f4-cake-h2 ~/regicide-match/results/bl-final* 2>&1 | head
grep -n "DLL\|dll\|evalfile" /mnt/c/regicide-match/jobs/booklab/f4_run.sh | head -8
