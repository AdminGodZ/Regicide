# Time use and a proposed test

Analysis only. No engine changes or matches were made.

## Accounting

For each side independently, start after the dealt opening with B0=0. Before own call j, A_j=B_(j-1)+0.5; after it,
B_j=A_j-t_j. A is the available bank including this move's credit; B is the balance after the preceding/current
call. Do not credit dealt opening moves or an unplayed next turn. The current runner computes:

- A<0.2: maxtime M=0.2;
-0.2<=A<=0.5: M=A;
- A>0.5: M=0.5+(A-0.5)/2.5 =0.3+0.4A.

The simple banking formula in the question is the last branch. maxtime is not a strict budget ceiling: Regicide
sets a separate hard limit, and the runner's CB mode can carry a negative balance rather than forfeiting then.

PDN times are rounded to0.01 seconds. The analysis accumulates each actual duration in [max(0,t-0.005),t+0.005];
bank_low/high and per-side entry/end balances are retained. All42 supplied compressed PDNs have times on every
played move and no expected times on the dealt prefix. All5124 games/556463 played actions replay through the
engine and independent referee. The primary four-engine comparison uses only the1872 final-validation n6c games;
old O and n8q/holdout records are kept separate. The24 per-run/engine move-count, mean and maximum checks match
the committed summaries within rounding. The old CSVs do not preserve exact per-game duration totals.

Clock basis: all these historical games predate PR49. The durations and bank debits above are runner wall time,
including preparation/pipe delay, not isolated getmove thinking time. PR49 now debits min(valid host time, wall)
while still printing wall in the PDN. Do not apply this PDN-only bank reconstruction to corrected-runner games.
Log wall, host/charged time, available bank, offered maxtime and after-call bank separately for the next test.
Both policies must use the same corrected host/runner and fresh controls.

KingsRow here used book mode1. Its future mode3/read-back runs are a new condition; do not pool them.

## What is measured

At non-instant (>=0.05s) roots with multiple legal choices, the median historical wall/maxtime ratios are Regicide0.375,
Cake0.761, KingsRow0.938 and GuiNN0.901. At20-24 pieces, means per such call are0.417/0.936/1.527/0.753 seconds;
the median fractions are0.404/0.729/0.879/0.908. Opponents often have longer book prefixes and hence larger banks;
these are descriptive conditional comparisons, not equal-position tests. Average leading book-marked calls in
these records: Regicide1.24, Cake6.47, KingsRow8.92, GuiNN4.70.

Median bank after own turn10 is2.01/1.79/3.17/1.03 seconds; at turn20 it is3.31/1.44/0.61/1.45. At later turns the
surviving games differ between opponents and most have entered the database; use the counts in bank_trajectory.csv.

At the first <=8-piece boundary, Regicide's median bank is3.28 seconds, Cake1.45, KingsRow0.97, GuiNN1.18. In the
same database-entering games, mean unused bank decomposes as:

| Engine | Bank at entry | Net bank gained afterward | Bank at end | Games |
|---|---:|---:|---:|---:|
| Regicide |3.32|15.08|18.40|1832|
| Cake |1.55|10.17|11.72|609|
| KingsRow |1.18|24.55|25.73|606|
| GuiNN |1.39|9.19|10.59|617|

Thus most final unused credit was earned after the middlegame. It cannot be relabelled as time available at ply15.
In final games ending above8 pieces, Regicide's median final bank is2.10 seconds; the three opponents are around
zero. The primary opportunity is a modest extra reserve spent on actual decisions, not reclaiming an18-second
end surplus in advance. Some opponents borrow against future credits under these rules.

The largest logged final-validation KingsRow wall duration is11.48s: b-h2 round16.2, full ply61,12 pieces,
available bank9.75s, M4.20s. The summary says11.479s. Other full-dataset maxima are Regicide2.51s, Cake6.14s,
GuiNN6.87s. The7.1s figure may use another clock/subset; old PDNs cannot recover getmove time. Maximum durations alone
conflate book credits, forced/instant moves, phase, policy and preparation/pipe overhead.

## Losses and the point of a score fall

loss_windows.csv retains two own actions before and after the first pre-database search score<=-50. It also
records the literal first score fall and the first>=50cp drop between search calls; the rule is explicit rather
than choosing a convenient turning point after examining the timing. Book scores are not treated as comparable
network search scores. The printed score is for the position before our move, not a proof of the played move's
value; the opponent's preceding reply can reveal the damage from our earlier move.

In the supplied holdout records EB loses78 times to Cake.69 already report<=-50 at the first logged search, so
these are not78 observed collapses from equality. Many H1/H3 starts are deliberately unbalanced. More time is
not known to repair a lost ballot, and this analysis does not prove those positions lost either. The nine remaining
EB/Cake cases and all other configurations/opponents have complete windows in the CSV. At the first threshold
crossing these78 losses used a median0.155s with median A=M=0.5; their median final bank is4.67s. That end balance
must not be projected back to the first call. all_loss_windows.csv supplies separate windows around all three
point definitions for every loss; the primary loss_windows.csv uses the <=-50 root definition.

For the five n8q/Cake losses, the move preceding the first<=-50 root used0.52/0.54/0.58/0.31/0.62s, with offered
maxtime1.316/1.312/1.296/0.972/1.144s and available credit2.54/2.53/2.49/1.68/2.11s. In #124 the visible fall is at
an obligatory capture; that root has only one legal move. The preceding unforced10-15 at18 pieces used0.31s for
a reported score-8, while a later deep analysis found an error. Spending more on that previous decision is a
testable idea, not proof that0.2-0.7 additional seconds would find the100M-node result. The committed deep analysis
was not rerun here and is not a certificate of the earliest losing move.

## Current search limits

For CB target time M, base soft=0.6M and hard=2.5M. Stability changes the soft stopping threshold to0.3M after six
unchanged best moves,0.48M after three,0.6M otherwise and0.96M after a fresh best-move change. These are iteration
boundary thresholds, not exact runtimes: an iteration can run past soft. A single legal root move stops after
one completed iteration; book/database roots can return instantly. Do not call that wasted search time.

tm_open_pieces=0 and tm_drop=0 disable both extensions by default. tm_open_factor=150 is defined but inactive.
The opening factor scales **both** soft and hard limits. The score-drop mechanism compares consecutive completed
iterations inside the same search, not consecutive own turns in a PDN. A between-turn score fall does not show
that tm_drop would have triggered, nor can it fix an earlier move once a forced reply exposes the mistake.

## Numerical policy and the cheapest useful test

The clean first probe needs no new engine implementation: baseline tm_soft60 versus candidate tm_soft100,
keeping tm_hard250, tm_open_pieces0, tm_drop0 and everything else fixed. On stable decisions this moves the
threshold from0.3M to0.5M, a67% increase. Examples:

| Available bank A | M | Stable threshold now | Probe | Same hard ceiling |
|---:|---:|---:|---:|---:|
|0.50|0.50|0.15|0.25|1.25|
|1.00|0.70|0.21|0.35|1.75|
|2.00|1.10|0.33|0.55|2.75|
|4.00|1.90|0.57|0.95|4.75|

With repeated stable unbooked calls and no iteration overshoot, current after-call bank equilibrium solves
B'=B+0.5-0.3(0.5+B/2.5), giving B=2.92s. The probe gives B=1.25s. It would invest roughly1.67s more during the
run of meaningful decisions instead of holding that reserve to the end. This is a model, not a replayed outcome.

A later phase-specific design could apply the larger soft allocation at>=17 pieces, covering the17-22-piece
region in the records, and leave hard unchanged. It must decouple phase soft/hard scaling. Setting the present
tm_open_factor167 with hard250 would instead raise the hard limit to4.175M, almost the runner's4M stop factor;
that is a different and riskier experiment. Do not advertise it as only spending soft time earlier. A separate
score-drop probe, e.g.30cp/factor150, should be tested separately after obtaining iteration logs.

The cheapest valid playing test is direct against Cake: one frozen policy contrast, fresh baseline
and candidate games from full ballots, matched opening/colour, balanced randomized order in short timing-locked
chunks, identical net/book/databases/options and banking mode, with BOTH arms using the corrected PR49 runner
and host. Log actual charged time and offered limits; the old PDN-only clock is insufficient. Selfplay --movetime
sets movetime_ms, which bypasses tm_soft; --target uses that branch but still lacks Cake and its banking behavior. A small20-40 matched-game diagnostic can reject a
bad idea, but cannot establish a small gain. Include draws/wins and conversion games, not just selected losses;
many of the observed holdout losses start worse already. Use n6c for the primary claim; n8q's five losses are a
separate diagnostic, not primary confirmation data.

If the pilot is promising, run a predeclared whole panel against Cake with fresh baseline controls and an
opening-level paired interval. For156 openings, both colours and two arms this is624 games. The H3 off-book
distribution is more informative than a production r11 panel whose early decisions may all be pinned; freeze
the book actually intended for deployment and report how many searches the policy could affect. Do not reuse old
baseline outcomes as the controls, tune on the confirmation, pool time controls or equalize node counts. Add
KingsRow/GuiNN and production regression only after the cheapest direct Cake test survives. There is no playing
strength conclusion from bank accounting alone.
