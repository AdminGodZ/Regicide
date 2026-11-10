date; uptime
echo "== processes of mine"; pgrep -af "datagen-tr|truth-tr|rescore-tr|pausekeeper_truth|regicide-jobs/truth/|jobs/truth/|train.py" | grep -v pgrep | cut -c1-120
echo "== locks"; ls -la ~/regicide-jobs/locks/ 2>&1 | cut -c1-90; ls -la ~/regicide-jobs/r4/matchlock 2>&1 | cut -c1-90
echo "== E5 files"; for f in gen6a_n50k_egdb gen6b_n50k_egdb; do s=$(stat -c %s ~/regicide-data/truth/$f.bin); echo "$f bytes $s mod16 $((s % 16)) records $((s / 16))"; done
echo "== status tail"; tail -n 8 ~/regicide-jobs/truth/status | cut -c1-150
echo "== worktree"; cd ~/regicide-truth && git status --short | head; git log --oneline -1; git fetch -q origin && git log --oneline -1 origin/t3/truth-data && git log --oneline -1 origin/master
echo "== egdb"; ls ~/regicide-data/egdb | wc -l; du -sh ~/regicide-data/egdb 2>/dev/null | cut -f1
echo "== bin"; sha256sum ~/regicide-jobs/truth/bin/datagen-tr5 ~/regicide-jobs/truth/bin/datagen-tr6 ~/regicide-truth/build/datagen 2>&1 | cut -c1-16,65-
df -h ~ | tail -1; nproc; free -g | head -2
