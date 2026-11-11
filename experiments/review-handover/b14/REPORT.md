# Regicide review, round 14, closed on stop order

All search/build/test activity was stopped when instructed. The active walk process, PID 69072, was killed;
its 100M forced query has no completed result. The controller ended and the stop sentinel remains in place.
No review search, build or test process remained when checked. No further engine/test executable, compiler, test suite or
checker was run after that point. Only saved results and committed records were read to prepare this report.

The requested analysis is partial: **24 of 67 won-game analyses completed; 43
are not done on this machine**. Exact next commands, positions, nets and budgets are in [UNFINISHED_SEARCHES.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/UNFINISHED_SEARCHES.tsv);
all remaining position contexts are in [REMAINING_POSITIONS.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/REMAINING_POSITIONS.tsv). Build/resume and non-search follow-ups are
in [REMOTE_RUN.md](/private/tmp/regicide-review-r1.tQRfyf/round14/REMOTE_RUN.md). This is not a sign-off on the unexamined cases.

## Findings with saved evidence

All three were reproduced before the stop order. The final fetched heads are master 4a3ad5278ad6115c13f828eac56109ccc7a1f11c,
Book Lab d029fd29d67277b407e8fda2726ad10f4b0683c7, truth-data b6e49b61481df654e89003cff439da55ebaa62ea. The affected source files were compared with
the tested copies and are unchanged. Execution revalidation of these final heads is **not done on this machine**
under the stop order. The evidence below names the commits actually tested; the fresh-head claim is source
identity, not an invented fresh run.

### 1. CONFIRMED: a successful setting change can leave get reporting the previous value

Tested master 5c433dce46da5e9b770a9d1b0ddd5fd7927fb7d6, also reproduced on 85a9fe8.
Severity: wrong option/configuration read-back. Frequency: rare, on an allocation failure during publication.
This is a remaining case in PR #61, not a repeat of the termination issue which that PR fixes.

At [master-current/src/cb_api.cpp](/private/tmp/regicide-review-r1.tQRfyf/round14/master-current/src/cb_api.cpp:87), publish constructs a fresh snapshot and catches every failure,
leaving the previous snapshot. The successful book setter changes book_mode and the Engine's book pointer,
then returns success; its publication destructor can silently fail. Get reads only the old shown.book_mode.
No getter retries publication when memory becomes available.

Input: first load the valid n6c file through the long pathname in [publication_stale.cpp](/private/tmp/regicide-review-r1.tQRfyf/round14/publication_stale.cpp). Then refuse ordinary
new allocations only during set book 0. The setter itself needs no allocation in this fixture; the one refused
allocation is the long string copy made by publication. Restore allocation and query get book.

Actual saved output:

    before get book: 3
    set book 0: rc=1 reply=0 refused=1
    after allocation recovers get book: 3

Expected: after an acknowledged change to mode 0, get book reports 0, or the change is refused before it is applied.
The existing catch prevents termination but cannot make the stale published state match a successful mutation.
The snapshot remains stale until another setter successfully publishes. [publication-stale-current.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/publication-stale-current.txt).
The original failed-hash reproducer now returns a normal error and continues with get hashsize=1:
[publication-fixed.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/publication-fixed.txt). No current crash is claimed.

### 2. CONFIRMED: the new won-line report certifies a game that it failed to replay

Tested Book Lab f13c61a3c77ff6d69330ae00fe7172fd2bc3c798, introduced with the new wins_vs_book tool.
Severity: wrong analysis/report result. Frequency: FEN-started games, or any input for which pinwalk returns ERR.
No FEN-started game is being claimed in the 67-game production inventory.

[booklab-current/tools/booklab/wins_vs_book.py](/private/tmp/regicide-review-r1.tQRfyf/round14/booklab-current/tools/booklab/wins_vs_book.py:29) drops the FEN tag; the input writer at line 49 always sends
an empty starting FEN. Lines 57-58 ignore ERR without removing the game. Lines 78-82 then interpret the empty
walk as no departure and print yes.

Concrete valid input is [coverage-fixture/fen-win/games.pdn](/private/tmp/regicide-review-r1.tQRfyf/round14/coverage-fixture/fen-win/games.pdn): Regicide is White and wins a complete
36-ply game from B:W21,K22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,K5,9.
Correct replay produces 16 own rows. The report replays from the standard board, receives ERR at ply 6 on 3-7,
discards it and exits 0 with:

    fixture (fen-win 2.1) | cake | W | 0 | yes

Expected: honour the saved FEN and require successful full replay before certifying a line; at least report the
failure instead of yes. Pinwalk's check=True cannot help: the bulk program deliberately emits per-game ERR
records with exit status 0. [coverage-fixture/fen-win.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/coverage-fixture/fen-win.txt),
[coverage-fixture/fen-replay-control.json](/private/tmp/regicide-review-r1.tQRfyf/round14/coverage-fixture/fen-replay-control.json), [coverage-fixture/fen-replay-control.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/coverage-fixture/fen-replay-control.tsv).

### 3. CONFIRMED: the avoid-entry repair fix still misses an exclusion inside a king-move run

Tested Book Lab f13c61a3c77ff6d69330ae00fe7172fd2bc3c798. This is a residual in fix 7a20295, not the already
handed-on case where the walk had no exclusion column.
Severity: wrong pin selection / lost-line explanation. Frequency: when a later own excluded move occurs with a
nonzero reversible counter. The current 8,112-game pin pool has no such occurrence; no current payload defect is claimed.

At [booklab-current/tools/booklab/pins.py](/private/tmp/regicide-review-r1.tQRfyf/round14/booklab-current/tools/booklab/pins.py:90), repaired_from still requires el for both a forced departure and
an excluded move. The same eligibility gate remains in lost_vs_book. Engine::go applies its active avoid list
outside analysis/database roots regardless of reversible_plies; only book consultation has the king-run gate.
The walk correctly reports the excluded move, but el=0 causes the repair to be discarded.

Constructed input: start B:W21,K22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,K5,9 and play
9-13 22-18 5-9. Before 5-9 the reversible count is 1. The base contains only an exclusion of 5-9.
The fixture has a valid max-three-ply draw plus a complete terminal Black loss of 36 plies. The earlier 9-13 has
non-lost support; its old loss should be ignored by the stated downstream-repair rule because 5-9 is unplayable
with the active exclusion. Instead the selector reports zero repaired occurrences and produces zero pins.

The public Engine check on that exact position/history excludes 5-9 and chooses 1-6 with 11 nodes.
There are other legal moves, so the all-moves-excluded fallback does not apply.
The normal later-candidate filter cannot correct an earlier lost-game veto; the assembler sees no erroneous pin
to reject. [king-fixture/console.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/king-fixture/console.txt), [king-policy-engine.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/king-policy-engine.txt),
[king_fixture.cpp](/private/tmp/regicide-review-r1.tQRfyf/round14/king_fixture.cpp), [king_policy_check.cpp](/private/tmp/regicide-review-r1.tQRfyf/round14/king_policy_check.cpp).
Only 48 positions were generated for these two terminal fixtures. A raw scan of the committed 52-run pool,
including ineligible positions, found no played excluded move: [avoid-impact.json](/private/tmp/regicide-review-r1.tQRfyf/round14/avoid-impact.json).

## PR #61 and built-in r11c verification completed before stopping

- The original allocation-refusal termination reproducer returns error: std::bad_alloc and continues.
- Saved master 85a9fe8 passed 49/49 ctest checks in 18.33 seconds. The later 5c433dc/n8q checkpoint passed 49/49
  in 25.40 seconds. No suite was run after the stop order. [ctest.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/ctest.txt), [ctest-current.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/ctest-current.txt).
- The 85a9fe8/n6c bench was 7,677,736 nodes. No claim is made that the new n8q net has the same bench.
- The embedded bytes, the file exported back through Book::builtin()->save, and Book Lab's book_r11c.bin were
  byte-identical: SHA256 14bc3e77d4df26a920d8ea60035fc578bbae316f84b1666e0ebde4aecbfad73f.
  The final fetched master's book file still has SHA256 14bc3e77d4df26a920d8ea60035fc578bbae316f84b1666e0ebde4aecbfad73f.
- All 81,061 entries and all five avoid entries were mapped to positions and resolved to legal moves.
  Their full move text parsed back to the same move. No unmapped or illegal entry remained. With distance tables enabled, a direct database choice may legitimately have zero nodes; MTC was disabled in the search-instead fixtures to distinguish that path from book consultation.
- All 46,978 forced decisions agreed with the payload; every forced key had exactly one book entry.
  No position had two different forced decisions.
- Through Engine, margin 0 corresponding to book mode 3, 2,400 pins across 309 opening/source groups chose
  exactly the pinned move with depth 0 and zero nodes. Twenty-four actual quiet-king predecessors of book
  positions were constructed; the resulting king-run positions searched instead. Three known win/draw/loss
  roots also searched with MTC disabled. The registered egdb_root check supplied hostile book/avoid entries
  to ensure the database-root guard was exercised rather than relying on an absent entry.

[book_check.cpp](/private/tmp/regicide-review-r1.tQRfyf/round14/book_check.cpp), [book-check.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/book-check.txt), [book-errors.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/book-errors.tsv), [pinned-sample.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/pinned-sample.tsv),
[book-positions.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/book-positions.tsv). The final-head source/artifact check used reading only; no new test execution occurred.

## Won-game analysis: completed portion

There are 67 wins in the explicitly requested campaigns: nine against KingsRow mode 3 and 58 against Cake.
Fifty-seven Cake wins are on the deliberately unbalanced T1 H3 panel. H3 is not a holdout for r11c; these are
retrospective line-retention observations, not a strength estimate. All 67 saved games were fully replayed before
the stop and ended with the opponent without a legal move. None passed an earlier threefold repetition or the
79-ply no-progress draw. [terminal-check.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/terminal-check.tsv), [terminal-check-summary.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/terminal-check-summary.txt).

The instrument is deliberately pinned to the requested 85a9fe8 with n6c, one thread, hash 128 MiB cleared per query,
book and exclusions off, seven-piece database cap, 20M and 100M node budgets. The later n8q merge and PR #65
were not silently mixed into those results. A completed point requires both returned root moves to differ from
the recorded move and the same alternative to score more than 30 centi-men better in separately restricted,
equal-completed-depth searches at both reference budgets. Table distances within an unchanged WLD result are not
called blunders. A larger search of the same engine is not the truth. Many T1 opponents are already strongly disfavoured at their first free choice; the reported preference does not certify a draw-to-loss transition.

Completed-iteration scores are kept separate from unfinished returned scores. One provisional H3 candidate was
withdrawn because the 100M returned move was the recorded move even though the previous completed PV differed.
Force-iteration traces may supply a lower common depth: limits.depth only controls the iteration loop's upper
bound. A direct control matched score, move and node count. Derived records carry derived_from; their timing/hit
metadata is not a fresh measurement. [iteration-control.json](/private/tmp/regicide-review-r1.tQRfyf/round14/iteration-control.json).

### Inventory and progress for every requested won game

| ID | Opening | Regicide | Opponent | Run / round | Search analysis |
|---|---|---|---|---|---|
| W0 | ACF #16 Wilderness II | W | kingsrow | bl-k3-kingsrow-a-h1 14.2 | not done on this machine |
| W1 | ACF #134 | W | kingsrow | bl-k3-kingsrow-a-h2 43.2 | completed: ply 37 |
| W2 | ACF #71 Diabolical Denny | W | kingsrow | bl-k3-kingsrow-b-h1 63.2 | completed: ply 25 |
| W3 | ACF #43 | W | kingsrow | bl-v11c-n6c-kingsrow-b-h1 39.2 | completed: ply 23 |
| W4 | ACF #43 | W | kingsrow | bl-v11c-n8q-kingsrow-a-h1 39.2 | completed: ply 23 |
| W5 | ACF #16 Wilderness II | W | kingsrow | bl-v11c-n8q-kingsrow-b-h1 14.2 | not done on this machine |
| W6 | ACF #71 Diabolical Denny | W | kingsrow | bl-v13-kingsrow-b-h1 63.2 | completed: ply 25 |
| W7 | ACF #163 | W | cake | bl-v13k4-cake-a-h2 69.2 | completed: ply 51 |
| W8 | ACF #71 Diabolical Denny | W | kingsrow | bl-v13k4-kingsrow-a-h1 63.2 | completed: ply 25 |
| W9 | 10-15 23-18 7-10 18-14 | B | cake | bl-t1a-60-cake-H3_0 2.1 | completed: ply 6 |
| W10 | 9-13 23-19 10-14 19-16 | B | cake | bl-t1a-60-cake-H3_0 10.1 | not done on this machine |
| W11 | 9-13 24-19 6-9 19-16 | B | cake | bl-t1a-60-cake-H3_0 11.1 | not done on this machine |
| W12 | 9-14 24-19 11-16 21-17 | B | cake | bl-t1a-60-cake-H3_0 14.1 | completed: ply 8 |
| W13 | 10-15 21-17 9-14 17x10 | W | cake | bl-t1a-60-cake-H3_0 16.2 | not done on this machine |
| W14 | 9-13 23-19 11-15 19-16 | B | cake | bl-t1a-60-cake-H3_0 17.1 | completed: ply 12 |
| W15 | 9-14 24-19 11-15 21-17 | B | cake | bl-t1a-60-cake-H3_0 18.1 | not done on this machine |
| W16 | 9-14 22-18 6-9 21-17 | B | cake | bl-t1a-60-cake-H3_0 19.1 | completed: ply 8 |
| W17 | 9-13 24-19 10-14 19-16 | B | cake | bl-t1a-60-cake-H3_0 20.1 | completed: ply 8 |
| W18 | 10-15 21-17 6-10 22-18 | B | cake | bl-t1a-60-cake-H3_0 21.1 | not done on this machine |
| W19 | 10-15 21-17 7-10 22-18 | B | cake | bl-t1a-60-cake-H3_0 24.1 | completed: ply 14 |
| W20 | 10-14 21-17 14x21 22-18 | B | cake | bl-t1a-60-cake-H3_0 30.1 | not done on this machine |
| W21 | 12-16 21-17 10-14 17x10 | W | cake | bl-t1a-60-cake-H3_0 38.2 | not done on this machine |
| W22 | 10-14 24-20 12-16 23-19 | B | cake | bl-t1a-60-cake-H3_0 41.1 | not done on this machine |
| W23 | 12-16 21-17 16-20 22-18 | B | cake | bl-t1a-60-cake-H3_0 43.1 | completed: ply 10 |
| W24 | 9-14 24-19 6-9 19-16 | B | cake | bl-t1a-60-cake-H3_0 46.1 | not done on this machine |
| W25 | 12-16 23-18 10-14 21-17 | B | cake | bl-t1a-60-cake-H3_0 48.1 | completed: ply 10 |
| W26 | 9-13 21-17 6-9 17-14 | B | cake | bl-t1a-60-cake-H3_0 49.1 | not done on this machine |
| W27 | 9-13 21-17 11-16 23-19 | B | cake | bl-t1a-60-cake-H3_0 50.1 | not done on this machine |
| W28 | 10-14 21-17 14x21 24-20 | B | cake | bl-t1a-60-cake-H3_0 51.1 | completed: ply 6 |
| W29 | 9-14 24-20 6-9 21-17 | B | cake | bl-t1a-60-cake-H3_0 52.1 | not done on this machine |
| W30 | 10-15 21-17 6-10 24-19 | B | cake | bl-t1a-60-cake-H3_0 53.1 | not done on this machine |
| W31 | 12-16 23-18 16-20 18-14 | B | cake | bl-t1a-60-cake-H3_0 56.1 | not done on this machine |
| W32 | 9-14 22-17 12-16 23-18 | W | cake | bl-t1a-60-cake-H3_0 57.2 | not done on this machine |
| W33 | 11-16 21-17 7-11 22-18 | B | cake | bl-t1a-60-cake-H3_0 60.1 | completed: ply 12 |
| W34 | 12-16 22-17 9-13 23-19 | B | cake | bl-t1a-60-cake-H3_0 61.1 | not done on this machine |
| W35 | 9-13 23-19 12-16 19x12 | W | cake | bl-t1a-60-cake-H3_0 63.2 | completed: ply 9 |
| W36 | 11-15 21-17 8-11 23-18 | B | cake | bl-t1a-60-cake-H3_0 65.1 | not done on this machine |
| W37 | 10-14 23-19 14-17 22x13 | W | cake | bl-t1a-60-cake-H3_0 70.2 | not done on this machine |
| W38 | 11-16 21-17 9-14 23-18 | B | cake | bl-t1a-60-cake-H3_0 75.1 | not done on this machine |
| W39 | 12-16 21-17 16-20 23-18 | B | cake | bl-t1a-60-cake-H3_0 76.1 | not done on this machine |
| W40 | 9-14 21-17 14x21 22-17 | B | cake | bl-t1a-60-cake-H3_1 7.1 | completed: ply 6 |
| W41 | 9-13 22-18 13-17 21x14 | W | cake | bl-t1a-60-cake-H3_1 9.2 | not done on this machine |
| W42 | 9-13 21-17 5-9 17-14 | B | cake | bl-t1a-60-cake-H3_1 10.1 | not done on this machine |
| W43 | 10-15 23-19 6-10 24-20 | B | cake | bl-t1a-60-cake-H3_1 14.1 | not done on this machine |
| W44 | 10-14 21-17 14x21 24-19 | B | cake | bl-t1a-60-cake-H3_1 16.1 | not done on this machine |
| W45 | 12-16 23-18 10-14 18-15 | W | cake | bl-t1a-60-cake-H3_1 18.2 | not done on this machine |
| W46 | 11-16 21-17 16-20 23-18 | B | cake | bl-t1a-60-cake-H3_1 19.1 | not done on this machine |
| W47 | 10-15 24-20 6-10 20-16 | B | cake | bl-t1a-60-cake-H3_1 22.1 | not done on this machine |
| W48 | 9-14 22-17 12-16 23-19 | B | cake | bl-t1a-60-cake-H3_1 27.1 | not done on this machine |
| W49 | 9-13 24-19 11-16 19-15 | B | cake | bl-t1a-60-cake-H3_1 28.1 | not done on this machine |
| W50 | 9-14 23-19 5-9 19-16 | B | cake | bl-t1a-60-cake-H3_1 32.1 | not done on this machine |
| W51 | 9-13 24-20 13-17 21x14 | W | cake | bl-t1a-60-cake-H3_1 33.2 | not done on this machine |
| W52 | 10-14 21-17 14x21 23-19 | B | cake | bl-t1a-60-cake-H3_1 38.1 | not done on this machine |
| W53 | 9-13 24-20 10-14 21-17 | B | cake | bl-t1a-60-cake-H3_1 40.1 | not done on this machine |
| W54 | 11-16 21-17 7-11 23-19 | B | cake | bl-t1a-60-cake-H3_1 42.1 | not done on this machine |
| W55 | 11-15 21-17 15-18 23x14 | W | cake | bl-t1a-60-cake-H3_1 45.2 | not done on this machine |
| W56 | 10-14 23-19 7-10 19-16 | B | cake | bl-t1a-60-cake-H3_1 51.1 | not done on this machine |
| W57 | 9-14 24-20 11-15 27-24 | B | cake | bl-t1a-60-cake-H3_1 52.1 | completed: ply 6 |
| W58 | 11-16 21-17 9-14 24-20 | B | cake | bl-t1a-60-cake-H3_1 53.1 | not done on this machine |
| W59 | 9-14 24-19 14-18 23x14 | W | cake | bl-t1a-60-cake-H3_1 60.2 | not done on this machine |
| W60 | 11-16 21-17 8-11 22-18 | B | cake | bl-t1a-60-cake-H3_1 61.1 | completed: ply 10 |
| W61 | 10-14 24-20 7-10 21-17 | B | cake | bl-t1a-60-cake-H3_1 65.1 | completed: ply 8 |
| W62 | 10-14 22-18 11-16 21-17 | B | cake | bl-t1a-60-cake-H3_1 66.1 | not done on this machine |
| W63 | 10-14 24-19 11-16 21-17 | B | cake | bl-t1a-60-cake-H3_1 67.1 | not done on this machine |
| W64 | 11-16 23-18 16-20 27-23 | B | cake | bl-t1a-60-cake-H3_1 70.1 | not done on this machine |
| W65 | 9-13 23-18 6-9 18-14 | B | cake | bl-t1a-60-cake-H3_1 76.1 | completed: ply 14 |
| W66 | ACF #71 Diabolical Denny | W | kingsrow | bl-t1a-60-kingsrow-h1 63.2 | completed: ply 25 |

### Qualified points and equal-depth scores

All scores are from the opponent's side to move. Each score pair is recorded move / alternative; these are
completed scores at the common depth, not a comparison of scores from different depths.

| ID | Ply | Recorded move → alternative | 20M-reference pair / depth | 100M-reference pair / depth |
|---|---:|---|---|---|
| W1 | 37 | 20-24 → 11-16 | -98 / -61 / d26 | -63 / -16 / d29 |
| W2 | 25 | 13-17 → 5-9 | -109 / -6 / d26 | -118 / -7 / d29 |
| W3 | 23 | 8-12 → 8-11 | -54 / -9 / d26 | -75 / -3 / d31 |
| W4 | 23 | 8-12 → 8-11 | -54 / -9 / d26 | -75 / -3 / d31 |
| W6 | 25 | 10-14 → 5-9 | -100 / -6 / d26 | -101 / -10 / d31 |
| W7 | 51 | 26-30 → 24-27 | -86 / -25 / d26 | -92 / -36 / d29 |
| W8 | 25 | 10-14 → 5-9 | -100 / -6 / d26 | -101 / -10 / d31 |
| W9 | 6 | 22-17 → 21-17 | -227 / -193 / d23 | -255 / -223 / d27 |
| W12 | 8 | 22-17 → 32-28 | -307 / -250 / d23 | -365 / -327 / d26 |
| W14 | 12 | 29-25 → 27-24 | -212 / -167 / d24 | -237 / -171 / d28 |
| W16 | 8 | 22-17 → 24-19 | -221 / -176 / d25 | -260 / -220 / d28 |
| W17 | 8 | 22-18 → 22-17 | -295 / -264 / d25 | -295 / -264 / d25 |
| W19 | 14 | 26-23 → 18-15 | -298 / -200 / d24 | -338 / -204 / d27 |
| W23 | 10 | 22-18 → 22-17 | -224 / -188 / d25 | -249 / -190 / d27 |
| W25 | 10 | 22-17 → 27-23 | -269 / -226 / d24 | -337 / -251 / d26 |
| W28 | 6 | 23-19 → 28-24 | -313 / -244 / d23 | -341 / -265 / d25 |
| W33 | 12 | 32-28 → 24-19 | -322 / -193 / d25 | -331 / -217 / d27 |
| W35 | 9 | 11-16 → 1-5 | -407 / -318 / d24 | -398 / -325 / d25 |
| W40 | 6 | 25-22 → 17-13 | -323 / -268 / d23 | -354 / -272 / d25 |
| W57 | 6 | 22-18 → 24-19 | -56 / -21 / d26 | -42 / -9 / d30 |
| W60 | 10 | 24-19 → 29-25 | -220 / -180 / d24 | -268 / -196 / d27 |
| W61 | 8 | 23-19 → 32-28 | -284 / -237 / d22 | -309 / -276 / d27 |
| W65 | 14 | 30-26 → 14-10 | -202 / -171 / d24 | -234 / -176 / d28 |
| W66 | 25 | 10-14 → 5-9 | -100 / -6 / d26 | -101 / -10 / d31 |

FENs of the qualified points:

- W1, ply 37: B:WK4,10,13,21,22,28,30,32:B1,2,3,11,15,20,23
- W2, ply 25: B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13
- W3, ply 23: B:W21,25,26,27,28,30,31:B1,4,6,7,8,14,19
- W4, ply 23: B:W21,25,26,27,28,30,31:B1,4,6,7,8,14,19
- W6, ply 25: B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13
- W7, ply 51: B:WK8,9,12,14,21,25,32:B1,3,7,23,24,26
- W8, ply 25: B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13
- W9, ply 6: W:W21,22,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,10,11,12,15,18
- W12, ply 8: W:W19,22,23,24,25,26,27,29,30,31,32:B1,2,3,4,5,6,7,8,10,12,20,21
- W14, ply 12: W:W21,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,16,19
- W16, ply 8: W:W18,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,11,12,15,21
- W17, ply 8: W:W21,22,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,13,14,20
- W19, ply 14: W:W18,24,25,26,27,28,30,31,32:B1,2,3,4,6,8,10,12,20,21
- W23, ply 10: W:W22,23,24,26,27,28,29,30,31,32:B1,2,3,4,7,8,9,10,11,20,21
- W25, ply 10: W:W18,20,22,24,25,26,27,29,30,31,32:B1,2,3,4,5,7,8,10,11,13,16,21
- W28, ply 6: W:W20,22,23,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,12,15,21
- W33, ply 12: W:W20,23,24,25,26,27,29,30,31,32:B2,4,5,6,7,8,10,11,12,16,21
- W35, ply 9: B:W12,18,21,23,24,25,26,28,29,30,31,32:B1,2,3,4,6,7,8,9,11,13,14
- W40, ply 6: W:W17,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,6,7,8,9,10,11,12,21
- W57, ply 6: W:W20,21,22,23,24,25,26,28,29,30,31,32:B1,2,3,4,6,7,8,9,10,12,14,15
- W60, ply 10: W:W22,23,24,26,27,28,29,30,31,32:B1,2,3,6,7,8,10,11,12,16,21
- W61, ply 8: W:W20,22,23,24,25,26,27,29,30,31,32:B1,2,3,4,5,6,8,9,10,12,15,21
- W65, ply 14: W:W14,24,25,27,28,29,30,31,32:B1,3,4,5,7,8,9,12,13
- W66, ply 25: B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13


### Book prefixes and observed recurrence

Book coverage is stricter than no forced departure. A missing entry requires search, and an entry in a king-move
run is not consulted. Those cases do not prove the search will leave the line. For example #43 White ply 22 has
no book entry and eight legal moves; Engine searched 52 nodes at depth 1 and still chose the recorded 29-25.
The four known different book moves over the full games are #134 ply 12, #16 ply 62, and #71 plies 44/50.
Only departures before the identified opponent point appear below.

The saved transposition-aware pool is the f13c61a snapshot. For KingsRow the main denominator is confined to the
known mode-3 requested campaigns and both T1 arms; older mode-1 records are not pooled into it. The target game is
excluded. Move counts are by occurrence; reached counts are unique other games. All individual occurrences are
in CLOSED_RESULTS.json. Newly committed r11d records were examined for identical move prefixes only after the
stop order. Their non-identical prefixes could transpose: that check is **not done on this machine**.

| ID | First book interruption before the point | First different eligible book move | Saved pool: other games reaching / matching opening+colour | Moves there | Later r11d exact-prefix matches |
|---|---|---|---|---|---|
| W1 | ply 12: 29-25 / 22-18; eligible 1 | ply 12: 29-25 → 22-18 | not done on this machine; prefix-only evidence in JSON | not done on this machine | {'20-24': 2} |
| W2 | none | none | 8 / 11 | {'5-9': 5, '10-14': 3} | {'10-14': 1} |
| W3 | ply 22: 29-25 / -; eligible 1 | none | 3 / 11 | {'8-11': 2, '8-12': 1} | {'8-11': 1, '8-12': 1} |
| W4 | ply 22: 29-25 / -; eligible 1 | none | 3 / 11 | {'8-11': 2, '8-12': 1} | {'8-11': 1, '8-12': 1} |
| W6 | none | none | 8 / 11 | {'5-9': 5, '13-17': 1, '10-14': 2} | {'10-14': 1} |
| W7 | none | none | 6 / 27 | {'26-30': 6} | {} |
| W8 | none | none | 8 / 11 | {'5-9': 5, '13-17': 1, '10-14': 2} | {'10-14': 1} |
| W9 | none | none | 3 / 3 | {'22-17': 3} | {} |
| W12 | none | none | 3 / 3 | {'22-17': 2, '22-18': 1} | {} |
| W14 | none | none | 3 / 3 | {'29-25': 2, '27-24': 1} | {} |
| W16 | none | none | 3 / 3 | {'22-17': 2, '24-19': 1} | {} |
| W17 | none | none | 3 / 3 | {'22-18': 3} | {} |
| W19 | none | none | 1 / 3 | {'26-23': 1} | {} |
| W23 | none | none | 3 / 3 | {'22-18': 3} | {} |
| W25 | none | none | 1 / 3 | {'22-17': 1} | {} |
| W28 | none | none | 3 / 3 | {'23-19': 3} | {} |
| W33 | none | none | 1 / 3 | {'25-22': 1} | {} |
| W35 | none | none | 3 / 3 | {'11-16': 3} | {} |
| W40 | none | none | 3 / 3 | {'25-22': 3} | {} |
| W57 | none | none | 2 / 3 | {'22-18': 2} | {} |
| W60 | none | none | 3 / 3 | {'29-25': 2, '24-19': 1} | {} |
| W61 | none | none | 1 / 3 | {'23-19': 1} | {} |
| W65 | none | none | 2 / 3 | {'30-26': 2} | {} |
| W66 | none | none | 8 / 11 | {'5-9': 5, '13-17': 1, '10-14': 2} | {'10-14': 1} |


[CLOSED_RESULTS.json](/private/tmp/regicide-review-r1.tQRfyf/round14/CLOSED_RESULTS.json) contains the full per-game record, previous search trace, positions, every saved
same-board hit, book gaps and later exact-prefix matches. [UNFINISHED_SEARCHES.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/UNFINISHED_SEARCHES.tsv) gives the precise next
query for the remaining 43 games, including the interrupted #16 query. There is no qualified first rejected move
yet for those games. Do not turn a pending candidate or an unchecked transposition into a negative result.

The later targeted #163 panel contributes six additional Cake wins outside the explicitly requested campaign
inventory. It was included in the saved recurrence pool, not silently added to the 67. The original #163 ply-51
point was not reached in those other lines. A different winning branch can exist without repeating that point.

## Later validation records

The r11c results below were both recounted and fully replayed before the stop order:

| Book | Net | Games | W / D / L | Replay |
|---|---|---:|---|---|
| r11c | n6c | 1560 | 22 / 1538 / 0 | complete |
| r11c | n8q | 1560 | 25 / 1535 / 0 | complete |

Latest committed r11d records were parsed after stopping, without board replay:

| Run | Scope | Games | W / D / L |
|---|---|---:|---|
| bl-v11d-n6c-cake-a-h1 | full validation | 156 | 0 / 156 / 0 |
| bl-v11d-n6c-cake-a-h2 | full validation | 156 | 0 / 156 / 0 |
| bl-v11d-n6c-cake-b-h1 | full validation | 156 | 0 / 156 / 0 |
| bl-v11d-n6c-cake-b-h2 | full validation | 156 | 0 / 156 / 0 |
| bl-v11d-n6c-guinn-a-h1 | full validation | 156 | 13 / 143 / 0 |
| bl-v11d-n6c-guinn-a-h2 | full validation | 156 | 12 / 144 / 0 |
| bl-v11d-n6c-kingsrow-a-h1 | full validation | 156 | 0 / 156 / 0 |
| bl-v11d-n6c-kingsrow-a-h2 | full validation | 156 | 1 / 155 / 0 |
| bl-v11d-n6c-kingsrow-b-h1 | full validation | 156 | 3 / 153 / 0 |
| bl-v11d-n6c-kingsrow-b-h2 | full validation | 156 | 1 / 155 / 0 |
| bl-w13-t8-r11d-cake | targeted | 32 | 0 / 32 / 0 |
| bl-w13-t8-r11d-guinn | targeted | 32 | 0 / 32 / 0 |
| bl-w13-t8-r11d-kingsrow | targeted | 80 | 0 / 80 / 0 |



Later parse totals: r11d n6c full validation: 1560 games, 30/1530/0; r11d targeted tests: 144 games, 0/144/0. These are counts only, not a completed legal-move replay.

Full legal-move replay of these later records is **not done on this machine**. The exact input and remote replay
command are provided in [later-records/replay-input.tsv](/private/tmp/regicide-review-r1.tQRfyf/round14/later-records/replay-input.tsv) and [REMOTE_RUN.md](/private/tmp/regicide-review-r1.tQRfyf/round14/REMOTE_RUN.md).
The parsed later files contain 0 Regicide losses. No completed loss analysis is claimed.
There is no invented position or command for an uncommitted future loss. The saved r11c recount/replay outputs
are [validation-counts.json](/private/tmp/regicide-review-r1.tQRfyf/round14/validation-counts.json) and [validation-replay.json](/private/tmp/regicide-review-r1.tQRfyf/round14/validation-replay.json); later parse results are [later-validation.json](/private/tmp/regicide-review-r1.tQRfyf/round14/later-validation.json).

## Lower-priority item

The new wins_vs_book column follows to the end means only that it found no eligible different book move or
excluded move. A book miss still prints yes. Renaming that column to no forced departure and reporting the first
book miss separately would make it match the current calculation. This wording issue is separate from finding 2's
failed replay being certified. Saved example: [coverage-fixture/bl-v11c-n6c-kingsrow-b-h1.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/coverage-fixture/bl-v11c-n6c-kingsrow-b-h1.txt) and
[coverage-fixture/engine-gap.txt](/private/tmp/regicide-review-r1.tQRfyf/round14/coverage-fixture/engine-gap.txt).

## Ruled out, limits and coverage

- PR #61 fixes the original termination path. The remaining issue concerns a successful setter, not a refused one.
- The r11c artifact is byte-identical, its legal moves and forced decisions passed, and all 2,400 sampled pins
  behaved as specified. No invalid built-in book entry is reported.
- No played avoided move appeared in the 8,112-game r11d source pool, including ineligible positions. The king-run
  repair residual does not establish a defective current pin.
- All 67 requested wins were genuine terminal wins under the saved separate check.
- N6c and n8q r11c validation had no loss. The later r11d WDL is a parse-only result until the postponed replay.
- The ledger's PR #65 table/history issue and truth relabel issue are not re-reported. H3's reuse in the pin pool
  is acknowledged; these are not holdout strength numbers.
- Fully read: PR #61 and affected publication/get/set paths, Book implementation, pin/avoid-repair diff and the
  new won-line report. Read the search iteration loop to justify saved-depth reuse. No new general core audit
  was started after the stop order.
- No compiler, test or search was invoked for closing this report. Current source identity checks used git reads
  only. The portable path adapter was written but not compiled or tested here.

Final source checks: [{"branch": "master", "path": "src/cb_api.cpp", "unchanged_from_tested": true}, {"branch": "t3/booklab", "path": "tools/booklab/pins.py", "unchanged_from_tested": true}, {"branch": "t3/booklab", "path": "tools/booklab/pinwalk.cpp", "unchanged_from_tested": true}, {"branch": "t3/booklab", "path": "tools/booklab/lost_vs_book.py", "unchanged_from_tested": true}, {"branch": "t3/booklab", "path": "tools/booklab/wins_vs_book.py", "unchanged_from_tested": true}].

Stop record: [STOPPED.json](/private/tmp/regicide-review-r1.tQRfyf/round14/STOPPED.json). Requested base master 85a9fe822fd9d92e27c8f4dbe325039ee737f6ec;
measurement build stayed on that base with n6c. The separate later interface check used
5c433dce46da5e9b770a9d1b0ddd5fd7927fb7d6. Book Lab code reproducers used f13c61a3c77ff6d69330ae00fe7172fd2bc3c798.
The final fetched refs are listed at the start. Further execution on this Mac is deliberately not authorised.
