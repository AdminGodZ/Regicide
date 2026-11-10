for p in $(pgrep -f "regicide-jobs/truth/losswalk.sh"); do kill $p; done; pkill -x truth-tr4; sleep 1
rm -f ~/regicide-jobs/truth/lw_*.tsv ~/regicide-jobs/truth/lw_*.tmp
bash /mnt/c/regicide-match/jobs/truth/launch.sh losswalk.sh
cp /mnt/c/regicide-match/jobs/truth/losswalk.sh ~/regicide-truth/experiments/truth-data/scripts/jobs/
bash /mnt/c/regicide-match/jobs/truth/tools/commit_push.sh "truth losswalk: rows carry the source path and both players; a bad FEN is a parse error; a capture written by two squares that fits two different captures leaves the game unresolved; move numbers follow the starting colour" tools/truth.cpp experiments/truth-data/scripts
