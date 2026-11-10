f=$(ls ~/regicide-match/results/tr-n8bn-cake-c0dde2e/games/ | head -1)
head -c 1600 ~/regicide-match/results/tr-n8bn-cake-c0dde2e/games/$f; echo
grep -n "0-1\|1-0" ~/regicide-match/results/tr-n8bn-cake-c0dde2e/results.csv | head -3
