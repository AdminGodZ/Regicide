import os
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
tab = open(os.path.expanduser("~/regicide-jobs/truth/pilot_draws.md")).read().strip()
block = ("### 4a (addition): draws at 9–12 pieces\n\n"
         "The lead asked whether n8q's over-confidence on drawn odd-piece endings (≤ 7 pieces, measured against the "
         "database by the reviewer) also shows where the engine has no database. `tools/pilot_draws.py`, no new searches.\n\n"
         + tab + "\n\n**Reading:**\n"
         "- **There are no proven draws at 9–12 pieces:** a search cannot prove a draw above the database. The draw rows "
         "are a proxy: positions the 200k-node search did not prove, from games whose first ≤ 8-piece position is a "
         "database draw. They can contain wins the self-play missed; the stricter proxy (200k score within ±20) shows the "
         "same pattern, smaller.\n"
         "- **Yes, at odd piece counts.** Drawn 9- and 11-piece positions are almost all one piece up, and n8bn and n8q "
         "score them further from zero than n6c: at 9 pieces mean |eval| 85–86 against 69, and 62% within ±100 against "
         "79%. At 11 pieces 82–88 against 75.\n"
         "- n8q is slightly closer to zero than n8bn at 11 pieces; at 9 pieces they are the same. Its ≤ 8-piece share does "
         "not cause the effect: n8bn, which never saw ≤ 8 pieces, shows it as well.\n"
         "- **Even counts (mostly equal material) are fine for all three:** 10 pieces 20–23 and 97–98% within ±100; 12 "
         "pieces 29–34 and 93–96%.\n"
         "- **The other side of it:** n8bn and n8q score proven wins and losses beyond ±100 far more often than n6c (9 "
         "pieces 98% against 92%, 12 pieces 90–92% against 86%). The database-relabelled targets make the nets more "
         "confident about a piece of material: right when it wins, too much when it draws.\n\n<!-- PHASE3MORE -->\n")
t = t.replace("<!-- PHASE3MORE -->\n", block, 1)
open(R, "w").write(t); print("ok")
