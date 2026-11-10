# Regicide holdout panels

These files hold **18 barred ballots, 19 cap-certified clean four-ply openings, and 156 touched four-ply openings**. The H2 filename retains the requested target `156`; its header and manifest state the actual 19. All certified clean cases at this cap are included. No static-evaluation cut was used.

Copy the top-level files to `experiments/holdout/`; do not copy `.work/`. It is ignored and contains immutable source archives, compiler logs and build products. The generator writes outside the source checkout. No match, engine alpha-beta search, package installation or network operation is part of generation.

## Reproduce

Requires Git, CMake, Ninja, Python 3.9+ and a Clang/C++26 toolchain able to build Regicide. The Mac validation used the repository's normal compiler and at most four build jobs.

```sh
python3 experiments/holdout/build_holdout.py --repo . --out /tmp/regicide-holdout --jobs 4
```

Defaults pin master `456e9b224a380771c50a1b0da9360757c16aa62e`, book source `3978ecd94f6f0f29a255b90d06a09eab9fb21c42`, and old master `fcf6acbce2bf3c0e211c6da27eea59b446cf6b51`. These commits must exist locally; the script never fetches. `--master-ref`, `--book-ref`, `--old-ref`, `--seed`, `--cap`, `--size` and `--cxx` are explicit overrides. The seed is **20261110**. Node caps and batch size are part of the panel definition: changing them can change the resolved pool and the sampled Tier 2. Freeze one manifest before matches and do not mix versions.

## Selection and contact rules

(a) Compare the canonical position after the first three plies with ballots mentioned by active decisions. This catches 48 source openings, including four alternate move orders beyond the 44 literal-prefix matches. (b) Compare every position from the initial board through the fourth ply with the decision positions: 12 openings. The source's 805 final positions are all distinct. Its 121 lines with a legal three-ply position outside acf174 are allowed and still checked by (b)/(c).

Narrow (c) follows our deterministic best book move and stops at an own book miss. The opponent can play any legal reply. It proves 17 contacts. Together (a)/(b)/narrow-(c) give **52 Tier-1 openings**, all listed first in H2x.

Strict (c) uses the book where the engine would use it, legal non-avoided moves on our side otherwise, and any legal opponent reply. Both assignments of our colour are checked. Book eligibility respects king-move runs. Every decision has at least nine pieces, so a path into the database-covered <=8-piece region cannot return to a decision position; no tablebase installation is needed for this check.

The position horizon is **depths 0 through 12 inclusive after the opening**. A contact certificate contains the forced action too, which can be action 13 at the boundary. The initial pass checks decision actions 1..12 with 50,000 uncached expanded states per colour. Only initially exhausted-clean cases need a boundary pass, with another 50,000 states for the opening's side to move; the other colour's possible goals already end at depth 11. A cap without a positive proof is unresolved and excluded. A proven existence of contact resolves a touched opening without needing to exhaust the other colour.

Material counts, directional men counts, and relaxed empty-board piece-distance lower bounds prune only impossible target reachability. Distances admit more capture paths than a real board and ignore assignment uniqueness, so they are lower bounds. The clean cache includes the exact piece sets, side to move, remaining horizon, our colour and book-eligibility bit. Draw-history restrictions are omitted from enumeration, making clean certificates conservative. All positive strict certificates were separately replayed through the referee with book-policy and no-past-draw checks. Short-depth results were also checked against unpruned complete enumeration.

The strict result has 19 overall clean openings, 174 known touched openings and 612 unresolved openings. H2x contains all 52 Tier-1 cases and a seeded sample of 104 from 122 proved strict-only contacts. Every Tier-2 record has a legal after-miss witness. Selection order is ascending SHA256 of decimal seed, a NUL byte, and canonical final FEN; evaluation values do not enter the ranking.

H1 contains the 18 kind-B ballots absent from acf156. None transposes to an acf156 final position. The source table gives no per-ballot reason or lost-side annotation. H1 is a source-ballot holdout and diagnostic, **not a strict-clean panel**: eight barred openings have proved strict contacts and ten remain unresolved. No H1 opening was dropped. The per-opening H1 contact tables and replayed certificates are retained.

## Validation and interpretation

All opening moves replay through both the independent tools/common referee and the engine parser, with board equality after every move. The actual match opening-file loader accepts each file. Every panel has unique final positions, and H2/H2x are disjoint. Static evaluations use the pinned master builtin n6c net, in Black's point of view for cross-panel summaries. They indicate skew, not proven outcomes; all openings are played with colours reversed. Inspect `manifest.json`, `opening_audit.tsv`, `witness_validation.tsv` and `SHA256SUMS`.

## Comparison plan

Freeze three configurations: O = fcf6acb with its old book; E = new master with the old book; EB = the same new master with the new book. Use the **same n6c net** in all arms, confirmed by checksum, so this is not a net experiment. Use the current corrected referee/runner for every arm, the same opponent versions/books/databases, one search thread, ponder off, hash 128 MB and book enabled with the default deterministic margin zero. The two source commits' old books and nets are byte-identical. An independent raw-book check found 975 changed position keys, all within active decisions; avoids are byte-identical.

Use 0.5 seconds/move in explicit CheckerBoard banking mode, matching prior external tests; do not mix exact or target semantics. Four predeclared repetitions per panel/configuration/opponent are a reasonable first block. That gives 144 H1 games, 152 H2 games and 1248 H2x games per configuration/opponent, 13,896 games for the complete three-arm/three-opponent block. Interleave arm order within panel/opponent/repetition blocks using the orders in `comparison_plan.json`, rather than completing every old-engine game first.

This H2 has only 19 independent roots. It is a pilot, not a well-powered 5-Elo equivalence study. Before an expensive general carryover claim, consider increasing the cap on the other machine to certify more independent roots, retaining the same rule; freeze that new panel version before play. More repetitions reduce run noise but do not create more opening units.

For each opening, form the colour-reversed game-pair score and matched increments: engine = E-O, book = EB-E, total = EB-O. Average repetitions and the three fixed opponents with declared weights. Resample opening clusters, keeping both colours and all configurations/opponents/repetitions together. Report opponents and H2x tiers separately. Primary endpoint is the pooled book increment on H2; secondary endpoints include the H2x increments and their interaction with H2. A 5-Elo effect near 50% is about 0.72 percentage points of game score; predeclare that practical margin if used.

Positive EB-E on H2 supports book benefit beyond the early contact window. Positive E-O on H2 with book benefit only on H2x supports engine generalization with localized book gains. Positive H2x alone does not prove failure to carry over if H2's interval is wide. To support no practically material carryover, H2's upper interval must exclude the predeclared material margin while the touched effect is resolved. No book repair or candidate selection may use these panel results before the confirmation is finished; doing so retires the panel as a holdout.

## Final ref check

The final fetch found master `c4d0ded90930c0587849568aaafdbf863cdfb6b1` after the truth-tools merge. Playing, parser/referee and inference sources were compared with the pinned panel-build master and are byte-identical; the source deck, four-ply file, net, candidate book and decisions also match. `comparison_plan.json` therefore names this current master for both new-engine arms while panel reproduction stays pinned to `456e9b224a380771c50a1b0da9360757c16aa62e`.

Tier-2 contact is an existence proof, not a prediction that a named opponent will play the witness. Record actual forced-decision exposure as a diagnostic, while retaining the predeclared panel/tier groups for the main comparisons. Do not select or discard games afterward based on observed contact.

## Addendum by the lead (2026-11-10): panel H3 and the plan actually played

The strict rule left only 19 certified-clean openings, too few to say much (38 games per run). Two facts change
what the holdout has to do:

- The new book differs from the old one in 975 positions, all of them forced decisions. In a game that never meets
  one of them, configurations E and EB play the same moves. The book can only matter in games that meet a changed
  position, and that can be read from the finished games.
- So the open questions are: how does the new engine do on openings outside the 156 production ballots at all,
  compared with the old one (any fresh opening serves), and what do the forced decisions do when a game reaches
  them from another opening (H2x).

`h3_unresolved156.txt` (built by `build_h3.py` from `opening_audit.tsv`) therefore takes 156 of the 612 unresolved
openings in the same seeded order. It is **not** certified clean. After play, every game is classified by whether
our side met a changed book position ("exposed") or not; the comparison of E and EB is reported for exposed games,
and the level of play (losses, wins) for all games.

A fourth configuration is added: **EBN** = EB with the net candidate n8q (`set evalfile`), because the net is to
become the built-in one and the book was validated with n6c playing after the book ends. The 805 source openings
were used for net-against-net selection, never against the three opponents.

Played first: O and EB on H1, H2 and H3; then EB and E on H2x; then EBN on H1, H2 and H3; E on H1/H2/H3 only for
openings whose EB games were exposed. One repetition; a second only where a result hangs on a handful of games.
