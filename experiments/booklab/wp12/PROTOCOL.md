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

- Points are taken from the games of the r11 validation against Cake with n6c (lines the pinned book really
  plays), not from older games whose prefix r11 would leave.
- At most three points per ballot and colour, one alternative each, deduplicated over all lines by position and
  move, at least four plies apart where possible. The known position below counts as one of the three on its
  lines if it passes the margin; if it fails the margin it is its own arm and does not count.
- Every ballot and colour from which a candidate's position is reached (by any opponent, in any validation game)
  is recorded with the candidate and kept through deduplication.
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
   Only wins in games that consulted the changed key count towards the 4; all 24 games are reported.
3. **Other opponents and other ways in.** The book does not know its opponent. For every kept candidate: 12 games
   against KingsRow and 12 against GuiNN from the nominating ballot and colour, and 12 games against each of the
   three opponents from every other recorded ballot and colour that reaches the position. A loss in a game that
   consulted the changed key rejects the candidate. A candidate whose key was consulted in fewer than 6 of the
   games against an opponent is reported as untested against that opponent, not as safe.
4. **Whole book.** r12 = r11 plus all kept candidates: two full runs of the production deck against all three
   opponents, and two runs with the unchanged r11 played alongside as the control (opponents vary between runs,
   so a win counts as mined only against the control's result on the same ballot and colour). No loss anywhere.
   Reported: wins against Cake with r12 and with r11, and for every kept key how often it was consulted, from
   which ballots, and the results of those games. If a loss removes a candidate, the failed version is kept in
   the report, the runs are repeated, and the final clean runs are called an acceptance screen of a selected
   book, not an independent validation.

## What the gates do not show

Twelve loss-free games bound a candidate's loss rate only at about one in five; a position reached from a second
opening, or after a rare reply, may be met twice in two full runs. The gates are a screen that keeps the claim
honest, not a proof that a pinned move never loses (review round A13, `PILOT_GATE_REVIEW.md`).

## Caching and resuming

Scores and discovery results are reused only for the same campaign: the cache key holds the engine build, net,
book and database hashes, the position, the node budget and the full move list; the discovery ledger holds the
hash of the candidate list. A game whose analysis fails is quarantined and analysed again, never marked done.

## Reporting

The funnel (points considered, candidates tried, discovery wins, confirmed, kept after stage 3, wins per run in
stage 4), one row per candidate in the template, and all game records committed. Mined wins are stated as
"learned production-deck book against Cake 1.89f at 0.5 s"; the holdout panels are not replayed for them.
