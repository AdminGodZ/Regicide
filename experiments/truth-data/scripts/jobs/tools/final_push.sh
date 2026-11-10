T=~/regicide-truth/experiments/truth-data; O=~/regicide-data/truth; J=~/regicide-jobs/truth
PY=~/regicide-nnue/.venv/bin/python
c=$(timeout 200 $PY /mnt/c/regicide-match/jobs/truth/tools/unique_count.py $O/gen6a_n50k_egdb.bin $O/gen6b_n50k_egdb.bin 2>&1 | tail -n 1)
python3 - "$c" <<'P'
import sys
p = "/mnt/c/regicide-match/jobs/truth/tools/res/RESUME.md"
t = open(p).read().replace("<!-- COUNTS -->", sys.argv[1] + " (game-deduplicated = records left after removing exactly repeated games; unique board+side keys measured at ~19:25: 7,334,907 of 10,748,672).")
open("/home/admingod/regicide-truth/experiments/truth-data/RESUME.md", "w").write(t)
P
timeout 120 $PY /mnt/c/regicide-match/jobs/truth/tools/lossreport.py > $T/logs/lossreport_partial.md 2>&1
mkdir -p $T/logs/losswalk; cp $J/lw_*.tsv $T/logs/losswalk/ 2>/dev/null
cp /mnt/c/regicide-match/jobs/truth/tools/lossreport.py /mnt/c/regicide-match/jobs/truth/tools/unique_count.py $T/scripts/ 2>/dev/null
cp /mnt/c/regicide-match/jobs/truth/*.sh $T/scripts/jobs/ 2>/dev/null; mkdir -p $T/scripts/jobs/tools; cp /mnt/c/regicide-match/jobs/truth/tools/*.py /mnt/c/regicide-match/jobs/truth/tools/*.sh $T/scripts/jobs/tools/ 2>/dev/null
python3 - <<'P'
R = "/home/admingod/regicide-truth/experiments/truth-data/RESULTS.md"
t = open(R).read()
s = open("/home/admingod/regicide-truth/experiments/truth-data/logs/lossreport_partial.md").read()
summ = s[s.index("### Summary by opponent and net"):] if "### Summary by opponent and net" in s else s
block = ("#### All book-off lost games against Cake and GuiNN (PARTIAL: 19 of 25 runs; stopped for a machine restart)\n\n"
         "Per-game rows: `logs/lossreport_partial.md`; raw output: `logs/losswalk/lw_*.tsv`. Read with the caveats at the top of 4b "
         "(no game history; the turn is a heuristic). KingsRow games are not part of it.\n\n" + summ.strip().replace("### Summary by opponent and net", "**Summary by opponent and net (partial)**") +
         "\n\n**Reading so far:** the games are proven lost at a median of 11 pieces, but the deep score falls below −50 much earlier:"
         " in most games while 20–24 pieces are still on the board, and in none at ≤ 8 pieces. That agrees with the reviewer's"
         " measurement that the search is nearly error-free one step beyond the database. The losses are made in the opening and"
         " early middle game.\n")
t = t.replace("<!-- LOSSTABLE -->", block, 1)
open(R, "w").write(t)
P
cd ~/regicide-truth && git add -A experiments/truth-data && printf 'truth-data: stop for machine restart: RESUME.md, partial loss analysis (19 of 25 runs), scripts\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\n' > /tmp/m.txt && git -c user.name=AdminGodZ -c user.email=159120313+AdminGodZ@users.noreply.github.com commit -q -F /tmp/m.txt
for i in 1 2 3; do git push -q origin t3/truth-data 2>&1 | tail -n 1; [ "$(git rev-parse HEAD)" = "$(git ls-remote origin refs/heads/t3/truth-data | cut -f1)" ] && break; sleep 10; done
git log --oneline -1; echo "remote: $(git ls-remote origin refs/heads/t3/truth-data | cut -c1-7)"; git status --short | head -3
echo "$c"
