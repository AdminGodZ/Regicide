for p in $(pgrep -f "regicide-jobs/truth/losswalk.sh"); do kill $p; done; pkill -x truth-tr4; sleep 1
rm -f ~/regicide-jobs/truth/lw_*.tsv ~/regicide-jobs/truth/lw_*.tmp
bash /mnt/c/regicide-match/jobs/truth/tools/build_truth.sh
cd /tmp && ~/regicide-truth/build/truth losswalk --egdb ~/regicide-data/egdb --nodes 100000 --threads 1 --out /tmp/lw2.tsv ~/regicide-match/results/tr-n8bn-cake1/games.pdn > /dev/null && cut -f3,6,8,19-24,27 /tmp/lw2.tsv | head -3 | cut -c1-220
bash /mnt/c/regicide-match/jobs/truth/launch.sh losswalk.sh
