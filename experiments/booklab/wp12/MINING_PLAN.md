# Win mining protocol proposed in round 12

This is a proposed experiment, not a result. No matches were run during this review.

## Freeze the baseline and the meaning of a candidate

Finish r11 validation with both nets first. Freeze the engine, r11 book, primary net n6c, opponent versions/books,
database files, one search thread, hash size, ponder off, CB book mode3, and 0.5-second CheckerBoard banking mode.
Record hashes, accepted option replies and immutable game files. Disable opponent learning if supported, or use
fresh immutable copies and record the starting state. The production book has no opponent identity.

A candidate is one **effective position/move change** against that frozen book, not a new filename or an added
duplicate row. Check its full Move identity and actual Book::probe result after applying all priorities. Existing
decisions remain protected; a candidate there needs a separately reviewed override, not a silently losing pin row.
Adding a pin at an old natural-book root is possible, but the explicit experimental override must survive the
assembly rule. Keep discovery variants separate; do not accumulate several new choices and attribute the result
to one of them. Integration changes the baseline and must be recorded as such.

Do not use wp11_build.sh as an assembly-only command after manually editing pins: it regenerates pins.txt first.
Keep an immutable base plus an explicit mining override file, precheck protected decision keys, apply the one
override to a copy of r11, and check that exactly one effective book key changed. Retain learned overrides as a
separate evidence-backed layer so a later regeneration cannot silently discard them.

Use full games from the ballot for discovery and confirmation. Starting from a FEN resets history, clocks, book
RNG and possibly the opponent's opening context; those are useful position tests but cannot stand in for full
production games. Keep the normal banking rule: different time use after the deviation is part of the book's
production effect. Do not pool banking and exact-movetime runs.

## Where to deviate

Priority1 is the shared root from WP7 recipes w30 and w31. They are GuiNN's two Black wins against Cake, on #163
and #165. After four actions both ballots reach the same position, key aec082fa8a86be0f. GuiNN plays8-12 at
one-based ply5; both final-validation n6c runs play11-15. Treat this as **one candidate key supported by two games**,
not two independent winning discoveries. That key was also reached in one final-validation KingsRow game, so
the cross-opponent risk is already concrete. No KingsRow-over-Cake win exists in the census.

The committed PDNs contain the chosen score/PV, not a comparable score for every alternative. They cannot certify
that8-12 is a near-equal second choice. Check it before adopting the proposed margin rule. If it fails the margin,
either exclude it from the primary campaign or declare a separate known-recipe arm; do not quietly widen the
margin after seeing its result. Do not copy a whole opponent-winning recipe and call it a one-move experiment.

For a broader pass, predeclare at most **three distinct own decision points per ballot/colour line**, one
alternative at each, deduplicated globally by position-plus-full-Move. Space points by at least four plies where
possible. Rank the earliest useful shared-recipe point first, then early book-eligible points near where the
validated lines first differed, then a distinct later middlegame point. Do not expand the per-line budget only
on promising results. Keep every attempted candidate in the denominator, including draws and rejected losses.

Eligible points must be before the first <=8-piece position, on our turn, at reversible plies0, with at least two
legal non-avoided moves and a real effective book change. Quiet king-run points cannot be pinned by this engine;
do not relax that safeguard for mining. Score the played move and alternatives with the book disabled, the same
fixed-node budget and cleared hash, and use completed search scores. Prespecify a margin such as15 centi-men and
reject known losing alternatives. The score margin is a heuristic filter, not a proof of safety or a probability
that the opponent will err. Log the scores, budgets, net and completed-depth/partial status.

## Discovery and replication

Discovery may try each candidate once and discard a loss or draw. A discovery win only admits a frozen candidate
to confirmation; it is not a retained win and contributes no confirmation sample. Exclude illegal-move, timeout
and crash wins from the learned-line success criterion, while retaining and reporting those failures separately.
A discovery win must include an eligible consultation at the changed key. Log unexposed wins, but do not
attribute them to that deviation or admit them as discovered successes. Use the exposure checker on the actual
frozen r11 and variant files, not the old975-key r10 list.

For each admitted candidate, screen with **12 new full-game candidate/control pairs against Cake at0.5 seconds**.
Continue only if at least one exposed board-play win repeats, with no safety loss or negative paired difference.
Then extend the frozen confirmation to **48 pairs total**, including those first12. Start fresh or demonstrably
reset processes, with AB/BA order randomized inside blocks. A proposed reproducible randomization seed is2026111012.
Split runs into short locked chunks, respecting the other machine's timing-lock limit; do not stop early for
success. An early safety loss can reject a candidate. Never restart a failed confirmation under a new name until
it passes. A screen with no win is unconfirmed, not proof that the candidate's win probability is zero.
For a key shared by several ballots, balance the assigned pairs across those contexts before play; do not pick
the context that happened to win most often. They remain one position-level discovery, not independent roots.

My proposed retention gate for the primary n6c condition is:

- at least10 exposed board-play wins in the48 assigned candidate games, occurring in at least three separately restarted blocks;
- no candidate losses;
- at least10 strictly positive paired score differences and no negative paired differences against untouched r11;
- no illegal/timeout/crash outcome counted as a board-play win;
- report all48 assigned games, including cases where the opponent avoided the changed root. Exposure-conditioned
  results may be a secondary diagnostic, not a replacement denominator.

This keeps a useful occasional win eligible rather than requiring an80% win rate, but does require repeated
evidence of improvement. Cap the reported confirmation campaign at50 admitted candidate configurations. Under
exchangeable randomized pairs,10 favorable non-tied signs and none unfavorable have one-sided probability2^-10
under no improvement. A union bound over50 is0.04883.
This is a conditional, fixed-opponent test, not an Elo estimate or a claim about new openings. Reset/learning
effects and serial dependence can invalidate the simple sign model; process isolation and random block order
are conditions of the interpretation. If more hypotheses or another primary net/TC are selected, adjust the
budget rather than resetting the comparison count.

Then run12 fresh matched pairs against KingsRow and12 against GuiNN on every known opening/colour context that
reaches the changed key. Run12 Cake pairs at1.0-second banking mode and12 Cake pairs using n8q. For a globally
deployed pin require no new losses or negative paired regressions in these screens. If a longer TC or n8q turns
wins into draws, report that interaction and call the improvement conditional; if it loses, reject the global pin.
Repeat analogous safety checks for n8q against KingsRow/GuiNN if the book is to be supported with both nets.
This secondary sample is a safety screen, not a statistically powered proof of equivalence or zero loss risk.

Even48 loss-free games leave a one-sided95% upper loss-rate bound of6.05% under an independent identical-trial
model.59 loss-free games are needed for a5% bound;299 for a1% bound, separately for each claimed condition.
Do not describe the small retention screen as proving that the move cannot lose.

## Global integration and the limits of pins

Make a reach index of all observed/transposed contexts for the candidate, including the opposite colour leg
where relevant. The current reach tool's positive certificates need the avoid-policy and witness-budget fixes
in the review notes; do not treat its current positive result as a complete deployment certificate. A negative
result from an exhaustive overapproximation is a different kind of statement.

After choosing a winner, regenerate the complete book, verify the exact delta, and run the full ACF156 both-colour
regression panel against all three opponents with both supported nets before promoting the combination. Conflicting
pins splice continuations: success of individual old games does not validate the assembled policy. In the present
r11 producer419 recorded traces first stop matching because the new book chooses a *different book move*, not
because it starts searching. Time-bank and first-search claims after such a fork require actual r11 games.

Opponent variation can leave the pin set immediately. There were300 opponent-first differences in312 KingsRow
cases and222 in312 GuiNN cases. A move that wins against Cake can be reached and refuted against another engine;
the shared candidate root already occurred against KingsRow. One observed loss is sufficient to reject a global
force in this conservative policy. An opponent-specific overlay would be a different product/configuration and
must not be described as the ordinary opponent-blind book.

## Expected yield

The external-opponent census gives Cake0 losses in312 games versus KingsRow and2 in312 versus GuiNN:2/624,0.32%,
or one loss per312 games as a **descriptive average**. GuiNN alone gives one per156. The two wins transpose to one
candidate root, so there is even less independent strategic evidence than two wins suggest. Cake self-play is a
separate control and must not be pooled into this external-opponent rate.

Against the final Regicide+n6c book there were0 Cake losses in624 games; against n8q there were0 in312. Those do
not establish a positive Regicide win rate. They provide no finite evidence-based forecast of games per new win.
The old census does not predict the yield of selected near-equal deviations.

For budgeting only, a constant independent probability2/624 would give312 discovery games per win in expectation,
and934 games for a95% chance of at least one win. That is an explicitly hypothetical model, not a forecast. The
targeted shared recipe may do much better or never reproduce. Confirmation/regression games come on top, and some
discoveries will fail confirmation. Begin with a fixed small pilot and report attempted candidates, actual reached
roots, discovery wins and confirmed wins; use that measured funnel for further budgeting.

## Reporting

Keep three separate result groups:

1. Frozen holdout: engine/book/net comparisons on the predeclared panels, with all assigned games and opening
   clusters retained. Mined trials never enter these rows.
2. Mining discovery: all tried candidates on the production deck, explicitly selected on these outcomes. No
   strength/Elo claim from the selected wins.
3. Fresh replication and whole-book regression: fixed candidate hashes and all assigned fresh games, with controls,
   opponent/net/TC interaction and exact exposure. Scope remains this deck and these opponent versions/TCs.

Use mining_report_template.csv. Each row should name campaign, stage, book/base hashes, net, opponent/version/TC,
ballot/colour contexts and unique position key, candidate budget/trials, assigned and actually exposed games,
candidate/control WDL, paired improvement, safety failures and retention decision. The visible scope column
should say "learned production-deck book; opponent/version/TC conditional", not "engine strength". Include failed
and drawn candidates, and show discovery and confirmation in separate rows rather than pooling them.

The original H2/H2x labels were defined for r10's decision set. r11 adds27418 position keys. Recompute exposure
from the two actual book files; H2's old bounded clean certificate is not automatically a certificate for r11.
Once any holdout outcome guides a book revision, retain its old frozen result but retire that panel as a fresh
confirmation set for the revised book. A successful mined book says nothing about fresh openings without a new,
untouched evaluation.
