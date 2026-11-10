# WP12 win mining: the protocol that is played

`MINING_PLAN.md` in this directory is the design proposed in review round A12; read it first. This file states what
is adopted from it for the pilot and where the pilot is lighter. The lead decides changes; nothing here is relaxed
after results are seen.

## What a mined win is

A forced book move, learned against one opponent version at 0.5 s/move in CheckerBoard banking mode on the
production deck (the 156 ACF ballots), that made that opponent lose where the validated line drew. It is a property
of the learned book, not of the engine's strength, and it says nothing about other openings. It is always reported
in its own table (`mining_report_template.csv`), never mixed with the holdout or with engine comparisons.

## Fixed for the whole campaign

- Baseline: book r11 after its validation with both nets; net n6c; master engine; one thread; hash 128; default book
  mode 3; opponents and database as in the final validation. Hashes of all of these in the report header.
- Opponent for discovery: Cake 1.89f. (Two validation runs against Cake differ first by our move in 158 of 312
  cases and by Cake's in 100, at a median ply of 32; KingsRow and GuiNN vary their own moves early, so a line found
  against them repeats less often. They are mined later, if at all.)
- A candidate is one effective change of one book key against the frozen book, applied as an override on an
  immutable copy of r11 (never by editing `pins.txt` and rebuilding). Existing decisions are protected.

## Candidates

- At most three points per ballot and colour, one alternative each, deduplicated over all lines by position and
  move, at least four plies apart where possible.
- A point qualifies if it is our move before the first position with 8 or fewer pieces, the book is consulted there
  (no run of king moves before it), and there are at least two legal moves that are not on the avoid list.
- The alternative is the best move other than the pinned one, by a search with the book off, a cleared hash and the
  same fixed node budget for every move (20 million nodes, completed iterations only), and it must be within
  **15 centi-men** of the pinned move's score. Scores, depths and budgets are logged for every point considered,
  kept or not.
- Order: (1) the known position behind recipes w30/w31 (GuiNN's two wins against Cake on ballots 163 and 165: 8-12
  where the validated line plays 11-15), as its own arm if it fails the margin; (2) points 10 to 25 plies after
  Cake's book ends; (3) earlier points.

## Stages and gates

1. **Discovery.** One full game from the ballot per candidate against Cake. A win nominates the candidate if the
   changed key was actually consulted in that game. Draws and losses end the candidate. Every attempt is logged.
   Pilot budget: the first 300 candidates in the order above; then a decision whether to continue.
2. **Confirmation.** 12 fresh games with the candidate and 12 with the unchanged book, interleaved, in at least
   three separately started blocks. Kept if the candidate wins at least 4, loses none, and wins at least 3 more
   games than the control. (The plan's stricter gate, 48 pairs with 10 wins, is not used for single candidates;
   the claim about the whole book is made by stage 4.)
3. **Other opponents.** The book does not know its opponent. For every kept candidate: 12 games against KingsRow and
   12 against GuiNN from the same ballot and colour. A loss in a game that consulted the changed key rejects the
   candidate.
4. **Whole book.** r12 = r11 plus all kept candidates: two full runs of the production deck against all three
   opponents. No loss anywhere, and the number of wins against Cake in those two runs is the result that is
   reported. A loss sends the responsible candidate back out and the runs are repeated.

## Reporting

The funnel (points considered, candidates tried, discovery wins, confirmed, kept after stage 3, wins per run in
stage 4), one row per candidate in the template, and all game records committed. Mined wins are stated as
"learned production-deck book against Cake 1.89f at 0.5 s"; the holdout panels are not replayed for them.
