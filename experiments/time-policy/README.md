# Time allocation: analysis and the test that follows from it

`TIME_POLICY.md` and the three CSV files are the analysis of review round A13 (2026-11-10), from the committed game
records of the final validation (1,872 games, net n6c, recorded times are the runner's wall times of that period).

What it found, in short:

- On moves that are really searched with 20 to 24 pieces on the board, Regicide used on average 0.42 s, Cake 0.94 s,
  KingsRow (book mode 1) 1.53 s and GuiNN 0.75 s; as a share of the time offered for the move, 40% against 73%, 88%
  and 91% (medians).
- When a game reaches a position with 8 or fewer pieces, Regicide has a median of 3.3 s in its bank, the opponents
  about 1 s. The time is left over in the phase where the games against Cake are decided.
- The reason is the soft limit: no new iteration is started after 60% of the offered time (`tm_soft`), and a
  stable best move lowers that further; the hard limit (250%) is rarely reached.

What this does not show: that more time changes results. An earlier test with four threads and pondering (several
times the search effort) against Cake changed nothing measurable on the production deck. So the proposal is tested
directly and cheaply before anything is changed.

## Test T1 (decided by the lead)

- Arms: `tm_soft` 60 (default) and 100, everything else equal: master engine, book r11 (after its validation), net
  n6c, one thread, hash 128, the runner and host of PR #49 for both arms. The parameter is set through the engine
  option `set param tm_soft=100` and read back.
- Opponent and openings: Cake on holdout panel H3 (156 openings, both colours: 312 games per arm), where results
  vary (the production deck against Cake is all draws and cannot show a difference). Arms alternate by part.
- Logged per move: wall time, charged time, bank before the move and the offered maxtime.
- Read-out: paired by opening and colour: wins, losses, score difference with an interval by opening; time used per
  phase and bank left at the first position with 8 or fewer pieces.
- Unit: match-score points (win 1, draw 0.5) over the 312 games of an arm. Three points are 0.96 percentage
  points. The interval resamples the 156 openings (both colours of an opening stay together); if a second run is
  played, the two runs are averaged within each opening first.
- Decision rule, fixed beforehand: adopt 100 only if (a) the paired score difference is positive with its 95%
  interval above zero, or (b) it is at least +3 points and a second run of both arms is positive again; otherwise
  the default stays. Path (b) is an adoption rule, not a 95% claim: a change accepted that way is labelled
  provisional, and the second-run and combined intervals are published. A result within noise is reported with
  its interval as "no measurable effect"; it is neither a tie-break in favour of the change nor proof that there
  is none.
- Before 100 is adopted or mining starts under it: one run of the production deck against Cake with the frozen
  book and the new setting. The deck is all draws, so it cannot show a gain, but it can show a regression; a new
  loss must be explained first.
- "Bank before the move" in the log includes that move's 0.5 s credit; the runner's own bank value is logged, not
  reconstructed from the offered time (review round A14, `T1_REVIEW.md`).
- The test is played before win mining starts, because lines found by mining depend on how the engine plays after
  the book ends.
