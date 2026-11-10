for p in $(pgrep -f "regicide-jobs/truth/losswalk.sh"); do kill $p; done; pkill -x truth-tr4; sleep 1
bash /mnt/c/regicide-match/jobs/truth/tools/build_truth.sh
cd /tmp
# reviewer's ambiguity example: B:W6,7,14,15,32:B2, black plays "2x18"
printf '[Event "t"]\n[Black "Regicide"]\n[White "X"]\n[Result "0-1"]\n[FEN "B:W6,7,14,15,32:B2"]\n1. 2x18 0-1\n\n[Event "u"]\n[Black "Regicide"]\n[White "X"]\n[Result "0-1"]\n[FEN "B:Wbad:B2"]\n1. 2-7 0-1\n' > /tmp/amb.pdn
~/regicide-truth/build/truth losswalk --egdb ~/regicide-data/egdb --nodes 1000 --threads 1 --out /tmp/amb.tsv /tmp/amb.pdn > /dev/null; cut -f1-8 /tmp/amb.tsv
mkdir -p ~/regicide-jobs/truth/lw_old && mv ~/regicide-jobs/truth/lw_*.tsv ~/regicide-jobs/truth/lw_old/ 2>/dev/null
cp ~/regicide-truth/build/truth ~/regicide-jobs/truth/bin/truth-tr4 2>/dev/null
bash /mnt/c/regicide-match/jobs/truth/launch.sh losswalk.sh
