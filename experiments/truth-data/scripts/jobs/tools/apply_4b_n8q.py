import os
R = os.path.expanduser("~/regicide-truth/experiments/truth-data/RESULTS.md")
t = open(R).read()
div = open(os.path.expanduser("~/regicide-jobs/truth/divergence.md")).read()
div = div[div.index("| ballot (Regicide)"):].strip()
conf = open(os.path.expanduser("~/regicide-jobs/truth/confirm_turns.md")).read().strip()
block = """### 4b: where the lost games were decided

**How to read the loss analysis** (`truth losswalk`, applies to everything below):
- The walk searches each position **without the game's history** and probes the database without the draw rules.
- **"Proven lost"** marks where a 10M-node search with the 8-piece database first proves the loss. That is a reliable
  upper bound on when the game was decided.
- **The "turning move"** is a heuristic: going back from there, the last of Regicide's decisions whose position still
  scored at least −50. It estimates where the game turned. It does not certify the first move that turned a draw into a
  loss.
- For the games that matter most the turn was checked by a large search (below).

#### The five games n8q lost to Cake with book r10 (Book Lab, bl-f4-cake-h1/h2; n6c drew all of them)

**At the first move where n8q leaves n6c's line** (`tools/divergence.py`):
- Both nets' static evals and searches are given, at the node count Regicide used in that game (×1) and ×10.
- Searches: this branch's CLI (master 456e9b2+ engine), `set evalfile`, 8-piece database, fresh hash, no history; a
  move's score is minus the score of the position after it.
- #37 and #75 transpose into the same position.

""" + div + """

- **No misjudgement at the divergence.** Neither net judges either candidate lost. n8q's move costs 10–40 centi-men
  against n6c's move in three of the four distinct positions (#87, #124, #131), and in #37/#75 it is the slightly
  better one.
- Re-searched without the game's history and hash, n8q's own search even prefers n6c's move in #124 and #131. So the
  in-game choice was close and depended on the search state, not on a clear evaluation difference.

**Where they were lost** (losswalk, 10M nodes per position; turning moves confirmed by 100M-node searches before and
after the move, with either net's evaluation):

""" + conf + """

**Reading:**
- **Proven lost:** #37/#75 at 12 pieces (move 21), #87 at 12 (move 37), #124 at 11 (move 25), #131 at 10 (move 39).
- **One clear error, at 18 pieces (#124).** The position before is level (0 with either evaluation); 10-15 drops it to
  −167 (n6c's evaluation: −107), while n8q's 0.5 s search had scored the move −8. That is a search-horizon miss; a deep
  search with either net finds it.
- **In the other games the estimated turning move was the best move** of the 100M-node search. The position was already
  somewhat worse (−30 to −80 at 17–22 pieces): those games slid from the opening phase, starting with the cheaper
  choices at the divergence (plies 6–15, 20–22 pieces).
- **This matches the reviewer's measurement:** one step beyond the database the search is nearly error-free. These
  losses are made early, with 17–22 pieces, by small concessions and once by a horizon miss, not at the database boundary.
- **Would E5-style data help here?** It covers exactly this region: seeds from plies 6–30 of real games against Cake,
  KingsRow and GuiNN, and 24% of its records have 17–24 pieces. But the errors are 10–40 centi-men choices between
  playable moves (one horizon miss aside), the kind of difference that lies within the noise of a net's evaluation. Better
  labels there may help a little; a verdict needs n9h/n9a against these ballots, not only self-play.

<!-- LOSSTABLE -->

<!-- PHASE3MORE -->
"""
t = t.replace("<!-- PHASE3MORE -->\n", block, 1)
open(R, "w").write(t); print("ok")
