# Regicide review ledger

This file is the one copy from 2026-11-11 08:14 on (before that it lived outside the repository). The lead edits it on
master; reviewers read it from `origin/master` and never edit it.

Kept by the lead. Both reviewers read this before starting a round and again before reporting. A finding that is
listed here as fixed, declined or handed on is not reported again unless the fix itself is wrong (then say so and
name the fix commit). Always test against the current `origin/master` (run `git fetch origin` first).

Current master: see `git rev-parse origin/master` (PR #73 merged 2026-11-11; built-in net n8q, built-in book r11d 7692986c...). bench 7,833,556. ctest 51/51.

## Fixed on master

| What | Where fixed |
|---|---|
| LMR table shared between engines in one process | PR #31 |
| Hash-move reference depended on list order / on the searched subset | PR #31, PR #33 |
| FEN ranges half-read, trailing characters accepted | PR #32 |
| Match tool ignored rejected engine options | PR #32 |
| SPRT variance floor (improved; still not error-controlled, documented, see "declined") | PR #32, PR #35 |
| No no-progress draw in the search | PR #33, refined in PR #36 |
| Restricted-root results stored as exact / upper bounds | PR #33 |
| bookgen frontier used the first path found | PR #33 |
| SearchResult had no way to mark an unfinished iteration (completed_score, partial, sound_score) | PR #33, PR #36 |
| Transposition shortcut ignored repetition of the child | PR #34 |
| enginecommand let exceptions escape | PR #34 |
| Loss claim / -32000 score from a search stopped before any move | PR #34 |
| set_evaluator kept the hash table | PR #34 |
| parse_move accepted non-jump middle squares; jumping the same piece twice | PR #34, PR #36 |
| Network output sum in 32 bits | PR #35 |
| Book consulted inside a run of king moves (bookgen backs scores through cycles) | PR #35 |
| Match deadlines started before the position was sent | PR #35 |
| No-progress / repetition adjudication before checking for a legal move (referee and search) | PR #35, PR #36 |
| Conversion search for database wins without distance information | PR #36 |
| Book or avoid list applied to a database-decided root | PR #36 |
| Loss claim to the GUI from a partial result | PR #36 |
| No-progress count missing from the hash key (exact from 48 plies; steps of 16 in the conversion search) | PR #36 |
| Static-eval pruning, ProbCut and singular verification near the 40-move limit | PR #36 |
| bookgen / datagen / selfplay / lossprobe took a partial losing score at face value | PR #36 |
| Book::probe let an entry with an illegal move take part in the choice | PR #36 |
| Late-move pruning inside the conversion search | PR #36 |
| Database setting change kept the hash table | PR #36 |
| Net loader: hidden biases without 32-bit headroom, output scale >= 2^31 | PR #36 |
| conversion_test: --sample not divisible by four hangs; sampled finished positions | PR #36 |
| Fixed time per move (movetime / exact mode) stopped early on a stable move | PR #37 |
| lossprobe: proved loss of the only alternative replaced by an older score | PR #37 |
| Referee rejected its own text for a single jump sharing end squares with a longer capture (illegal-move loss) | PR #38 |
| Referee rejected a capture written along the other route round a circle | PR #38 |
| CLI printed captures by end squares only; replay could play a different capture (move_text) | PR #38 |
| `truth fens` divided by zero on an input without a complete record | PR #41 |
| Root-policy explanation (comment and report) left out the capture case | PR #41 |
| cbhost lost a `stop` that arrived before the engine was entered (possible timeout loss) | PR #49 |
| CheckerBoard time bank charged the runner's wall time (history command, pipe) instead of the getmove time | PR #49 |
| Incremental time control still debited and forfeited on the runner's wall time | PR #51 |
| cbhost: `quit` and end of input did not cancel a go that was queued or being prepared | PR #51 |
| Database generator cursor read one men configuration beyond its table after the last entry of a subslice | PR #53 |
| `truth pilot` crashed on an input without a complete record; sampled forever without a record in the piece range | PR #53 |
| cbhost: `quit` / end of input stopped only the go read last, an earlier queued go ran in full | PR #53 |
| Opening loader: a parenthesis inside a PDN { } comment dropped the moves after the comment | PR #54 |
| Match runner: a game interrupted together with its engines was recorded as a loss by crash | PR #54 |
| cbhost: one pending stop only; a second queued go's stop overwrote the first; a stop ended an earlier running go | PR #54 |
| Opening loader rejected PDN lines starting with '%' | PR #55 |
| Book / avoid list applied at a root the database knows as a draw (could pick a losing move) | PR #56 |
| Hash resize freed the table before allocating; a failed allocation left no table | PR #56 |
| parse_move (engine and referee): long digit runs overflowed and could be read as a legal square | PR #56 |
| Database views were not released when the library is unloaded | PR #57 |
| `set dbpath` with another directory than the loaded one was acknowledged but ignored | PR #57 |
| Engine::set_threads dropped the workers before the new ones existed (crash after a failed allocation) | PR #57 |
| Match runner: braces in an engine's status text ended the PDN comment | PR #57 |
| rescore / selfplay: a worker thread called std::exit (static destructors under running searches) | PR #58 |
| selfplay: a game ended by the move on the last allowed ply was scored as a draw | PR #59 |
| cb_api: engine set-up outside any lock (two concurrent first calls) | PR #60 |
| cb_api: `get` read option fields unlocked while `set` wrote them (now a published copy with its own lock) | PR #60 |
| Net loader accepted feature-transformer sums that leave the 16-bit accumulator | PR #60 |
| cb_api: publishing the option values could throw out of a destructor while a refused setting was reported | PR #61 |
| Table move at a database-decided root ignored the game history (a lost root did not take a draw by repetition) | PR #65 |
| match: engines are kept per worker; --fresh-engines and PDN tags Worker / EngineGames added | PR #66 |

## Declined or accepted as approximations (do not re-report; say so only if you can show a wrong game result)

- SPRT error rates in draw-heavy runs: decisions use fixed sample sizes; documented in --help.
- Zero-width Elo interval when every pair is drawn (cosmetic).
- A database win whose conversion comes after the 40-move limit is still shown as a win.
- A root handed to the engine after the 40-move limit is searched as usual.
- Slice files with a matching checksum are trusted (no validation of offsets inside them).
- Two-fold repetition scored as a draw inside the search, and hash entries that depend on game history.
- The no-progress count is not in the hash key below 48 plies outside the conversion search.
- The playnow flag is a plain int written by the host (interface cannot change).
- Engine::egdb_root_move counts every man move as a conversion, which is not the tables' definition (capture,
  promotion, advance of the leading rank). Reviewer B, round 4, specified the distance-optimal rule and checked
  both exhaustively on all 3- and 4-piece positions: the present rule never loses the win and cannot shuffle
  (it has its own decreasing measure), and the "optimal" rule does not give shorter games overall (it was
  longer about as often as shorter). Decision: keep the present rule.
- The conversion evaluation values a king at 400 and a man one step from crowning at 390, and adds a distance
  penalty for each king, so crowning can look bad statically (reviewer A, round 5). MEASURED by reviewer B in
  round 6 (king 600, king 680, a proximity term; 20,160 games on 5-piece and real 7-piece tables at 5,000 and
  20,000 nodes): no clear gain in conversions (600: 110/120 = baseline at 5,000, 117 vs 115 at 20,000; wins about
  8 plies shorter), none gave a win away. Decision: no change; closed unless match games show failed conversions.
- Link-time optimisation does not reach regicide_core (the target is created before the default is set). Measured
  2026-11-10: enabling it gives the same bench node count and no speed difference (3.82M vs 3.84M nps over six
  alternating runs); left as it is because the same build file produces the Windows DLL.
- Small time overruns (about 1 ms; hash clear before the timer starts; node limit with several threads is a
  proxy): not hard real-time by design.

## Merged housekeeping

- PR #39: the ten regression tests of review round B5 (ctest names lmr_isolation, hash_clear_evaluator,
  hash_clear_egdb, book_king_run, egdb_root, nnue_bounds, sound_score, fixed_movetime, match_rejected_options,
  match_deadline) and its sixteen documentation / dead-code items.

- PR #40: the tool side of t3/truth-data up to 5bbd6d8 (tools/truth.cpp, tools/rescore.cpp, tools/label.hpp,
  datagen with the database and game-start flag, record flag bits in src/datafile.hpp, trainer group sampling and
  exact sums, experiments/truth-data). Built-in net unchanged (n6c). Round A8's three code findings on that branch
  (pilot bypassing label_of, np.bitwise_count, float64 sums on MPS) were fixed before the merge (dc8ffcb). This
  code is now master and open for review like the rest.
- PR #42: experiments/holdout/ (opening panels H1, H2, H2x by reviewer A's round 10, unchanged; plus the lead's
  h3_unresolved156.txt / build_h3.py and a README addendum). Data and scripts only.
- PR #43: seven more regression tests from review round B6 (ctest names illegal_book_entry, count_key_boundaries,
  etc_repetition, conversion_lmp_disabled, match_terminal_before_draw, bookgen_frontier, bookgen_sound_score);
  shared database fixture builder tests/test_egdb.hpp.
- PR #45: four regression tests from review round B7 (limit_rfp, limit_probcut, limit_singular,
  datagen_partial_loss).
- PR #44: t3/booklab merged, book r10 built in (987 decisions; payload verified by reviewer A round 11: legal,
  unambiguous, rebuilds byte-identically, r8 minus exactly four decisions; branch items movetext single jump,
  theory table and unfinished games, force-versus-avoid conflicts settled). tools/booklab and experiments/booklab
  are now master and open for review like the rest.
- PR #46: experiments/holdout/EXPOSURE.md, classify_exposure.py, exposure_helper.cpp (reviewer A's exposure rule).
- PR #47: regression test lossprobe_partial_loss (round B8); warning fix in tools/booklab/reach.cpp.

## With branch owners (fixes appear on those branches)

- origin/t3/truth-data: build_win.sh publishes a stale DLL after a failed build; game-boundary heuristic and game-start flag, mixed legacy/flagged files, rescore sentinel
  and proven-loss rule, float32 sums in quantisation-aware training at FT 2048, export check gate in train.sh,
  empty group in phase-balanced sampling, datagen printf field.
- origin/t3/booklab: reach.cpp applies the book where the engine skips it (king-move runs, database-decided
  roots); three unreachable decisions, the #34 ply-44 decision, first-occurrence rule in assemble.sh; capture
  identity in move text, equivalent circular routes, repeated jump in movetext.hpp,
  census pairs and "n/a", book-move detection, dbscan (unfinished games, dedup order, dedup key without result,
  partial replay after an illegal move, worker exit status), bl_run.sh resume guard for fixed-colour decks,
  "scheduled" total with fixed-colour openings, wp9.py report (no check that every planned position appears
  exactly once; no check of colour, opponent and starting board of a game against its planned case).
- origin/t3/truth-data, round A11 (sent to the owner 2026-11-10 ~17:25; the code is also on master, the owner fixes
  it on the branch): datagen restart with the same seed repeats the same games; append accepts a file with a
  partial trailing record; e5_gen.sh runs a stale binary after a failed copy; `truth split` loses game-start flags
  when a game's first record is filtered out; trainer validation cut falls inside a game and positions can be on
  both sides of the split.
- origin/t3/booklab, round A11: tools/booklab/wp10_report.py counts king-run positions as exposed (rule in
  experiments/holdout/EXPOSURE.md); follow-up games selected on exposure are descriptive only.
- origin/t3/booklab, round B10 (sent 21:09): bl_run_w.sh runs its pre-flight (which loads our database) before
  taking the timing lock; it releases the lock before writing the completion marker; pins.py's lost-game guard
  checks the position, not the chosen move; cbshim_report.py counts warm-up searches as played moves.
  Verified in round B10: wrapper failure propagation, failed-start retry, losswalk single jump (truth-data
  74e7755), KingsRow mode-3 probe and both mode-3 runs (2/307/3, 1/307/4), the committed Windows toolchain.
- origin/t3/booklab, round A13 (sent 20:52): mining tools: wp12_discover.py marks every game analysed after an
  exception (generator passed to len); wp12_select.py score cache keyed by point id only; the known position is
  not counted towards the three-per-line cap; wp12_points.py takes points from old games past an r11 departure.
  Residuals: reach.cpp horizon when the ballot is the target; pins.py statistics count rejected games and miss
  the eligibility check in the coverage clause. C1, C3, C4 (pins), C5 (headline), C6 verified as fixed.
- MATCH CONFIGURATION, round B9 (sent 19:54): KingsRow ran with book mode 1 ("all kinds of book moves") in every
  match so far; Cake, GuiNN and Regicide with 3 ("best moves"). The book agent re-plays the production deck
  against KingsRow with `set book 3` and sends an explicit book mode with read-back from now on. Earlier
  KingsRow numbers are valid for mode 1 only.
- origin/t3/booklab, round B9 (sent 19:54): wp10_report.sh ignores the reporter's exit status; bl_run_w.sh /
  bl_run_x.sh treat a failed start as a finished run; pins.py pins a move seen only in lost games.
- origin/t3/truth-data, round B9 (sent 19:54): losswalk's ambiguity check rejects a plain single jump that shares its
  end squares with a longer capture.
- tools/booklab on master + origin/t3/booklab, round A12 (sent 18:49; the owner fixes on the branch): reach.cpp
  ignores the avoid list for own moves after a book miss; reach.cpp witness mode ignores the ply horizon;
  bookapply.cpp reads past a 12-byte version-2 header; pins.py / pinwalk.cpp take pins from unfinished or
  partly invalid games; pins.py "first search with pins" statistic mistakes other book moves for searches.
  r11 payload (27,418 pins, sha256 2509043e...) verified for integrity and selection rule; 57 ties fall to the
  first move met (to be documented).
- origin/t3/truth-data, round A12 (sent 18:49): 915dcfb lost the PR #41 guard again (`truth fens`, empty input);
  the five pipeline fixes of ba45aca are verified; "distinct positions" counts records after exact-game
  deduplication, not unique positions.
- origin/t3/booklab, round B8 (sent 18:25): wp10_report.py pairing keeps one game per configuration and overwrites
  repeats (paired differences depend on --runs order); wp10_report.sh reports games of unfinished runs without
  exporting their records. The king-run exposure fix is verified.
- origin/t3/truth-data, round B8 (sent 18:25): losswalk identifies games by file basename only, ignores an invalid
  FEN, guesses on an ambiguous end-square capture, prints a wrong move number for games starting with White.
  Its "turning point" is a heuristic and searches ignore game history (stated in the report, not a defect).
- origin/t3/search-lab, round B7 items: FIXED in a5a6600 and verified in round B8 (runner completeness, posbench
  snapshot, chunk partition of the 805 openings).
- origin/t3/search-lab, round B7 (sent to the owner 2026-11-10 ~17:00): netm/runner.sh accepts interrupted chunks
  as finished and ignores worker / pooling exit status; posbench --ttd takes score and database hits from later
  work than its nodes and time; the six chunk opening files are not committed (partition of the 805 openings
  unverified). The evaluator-change clearing and both smp_vote fixes of 01b02b0 are verified.
- Net candidate n8q (truth-data bd1c5c0), round B7: file identity, loader, export check, incremental vs refresh,
  colour symmetry and arithmetic headroom verified. Measured, not a defect: on database draws with 5 and 7
  pieces its static evaluation is far from zero (mean 178 / 127 against n6c 94 / 83); the owner reports the
  same statistic for 9-12 pieces.
- origin/t3/search-lab (older items, fixed in 01b02b0 except as noted above): posbench --ttd overruns the target depth,
  evaluation cache survives an evaluator change, smp_vote (a helper result is vetoed when the main thread has no
  completed iteration; when the main thread is selected its unfinished result is returned instead of the
  completed snapshot). tm_nodes was reviewed and found consistent.

## Round 15 (2026-11-11)

- Handed to Book Lab (not master): `expected_games()` in experiments/booklab/scripts/bl_run_w.sh counts a mixed deck
  (fixed-colour and two-colour rows) wrongly (3 games, expects 4). Still open there at eab8514: t1_report.py bank
  reconstruction, wp12_discover.py replaying a quarantined game, wp12_select.py database identity by name and size,
  cap/spacing checked on the nominating line only. Do not re-report these; report only when a pushed fix is wrong.
- Ruled out: trainer resume with a different --out-scale (the README requires the same arguments on resume).
- Time-management path for CheckerBoard matches (bank formula, tm_soft/tm_hard thresholds) read and found as
  documented in experiments/time-policy/.

## Round 16 (2026-11-11)

- Whole ctest suite and bench under the address and undefined-behaviour checkers at 235d3e0: no diagnostics.
- Known and accepted: the match tool has no native resume (a rerun into the same directory starts again); the
  wrappers replay whole runs.
- `truth pilot` guard has no registered test yet (with the truth-data branch owner).

## Round 17 (2026-11-11)

- Accepted (do not re-report): after a failed engine call the match runner waits 50 ms and files the game as
  interrupted if an interrupt has arrived; a real crash followed within 50 ms by an unrelated interrupt is left out
  too. Interrupted runs are never counted as complete, so no result depends on it.
- Accepted: ';' rest-of-line comments in PDN openings are not supported.
- Handed to the truth-data owner: lossreport.py treats an in-game score of 0 as missing (seven committed rows;
  horizon counts Cake/n6c 6->9, Cake/n8bn 3->6, GuiNN/n6c 0->1); train.py / nnue.py report validation loss 0 when
  the game-boundary cut leaves no validation records; truth.cpp losswalk takes a score comment from inside a
  variation.
- Handed to the search-lab owner: netm/validate.py and pool_dirs.py accept duplicated game rows (same copy of
  pool_dirs.py in truth-data); runner.sh marks a failed or short sweep done and row.py drops the partial label.
- Handed to Book Lab: wp13/book_r10k.report.txt and .reach.txt still describe try 1. Verified at 10ddc05: the five
  owner items fixed; accepted r10k (229de021...) rebuilds byte-identically, 72 additions legal and reachable,
  force-unavoid removes exactly the stated entry; targeted results and the Cake-KingsRow mode-3 census reproduce.

## Reviewer B round 11 (2026-11-11)

- Handed to Book Lab: wp12_discover.py imports saved rank-named results (bl-w12-d-cNNN, books/cNNN.bin) of an older
  campaign into a fresh one (namespace by campaign identity or verify a manifest); override.cpp reads version-1
  book entries through a misaligned pointer; cbshim_report.py keys FEN-started games differently in trace and PDN,
  so the warm-up is counted instead of the first real move.
- Search opinions recorded (not defects): WP13 repairs 3-7 (#71) and 1-6 (#34 ply 19) are clearly supported by 20M
  and 100M searches of both nets; 7-10 (#60) mixed; 6-9 (#141/#9 ply 7) and the un-avoided 16-19 (ply 11) are not
  supported by the deep searches (11-15 preferred by 35 / 20 at depth 28) but by 36 drawn games.

## Round 18 (2026-11-11)

- Handed to Book Lab: theory.py reads variation moves as main-line moves; theory.py path pinning (--pin-path /
  repair_all.py --pin-only) forces a move whose only supporting game was lost; theory.py prints the source game's
  ply as the repaired ballot's ply; pins.py does not check that exactly one side is Regicide (an opponent-only
  run yields "regicide" pins); the #93 decision text quotes a score (2-6 -70/2) measured at ply 17 for the ply-19
  position.
- Handed to the truth-data owner: pool_dirs.py (fix of R17-2) rejects valid pairs when both engines have the same
  display name.
- Read and found correct (Windows host and DLL): calling convention on x64, CBmove layout (268 bytes), board
  mapping, DLL loading and working directory, reply buffers, serialised getmove calls, timing flags, file mapping
  sizes. Not certified: install paths longer than MAX_PATH or non-ANSI (GetModuleFileNameA) - accepted.
- Accepted: a failed first `set dbpath` still answers 1 with "0" pieces (callers read the count back).

## Round 19 (2026-11-11)

- Handed to Book Lab: t1_report.py compares whatever game files exist (failed or short runs enter the paired
  comparison); its time-forfeit count includes an opponent's timeout; pins.py still accepts a game whose other
  player has an unknown name as Regicide's (theory.py family() falls back to "regicide"); RESULTS.md text on the
  16-19 line (minimum score -29, not -11; 28 or 48 games, not 25) and on the ACF 55 comparison (first difference
  from the r10k4 repeat is KingsRow's ply 45).
- Verified at booklab ddc4878: path-pin result guard, variation skipping, ballot ply, campaign-keyed discovery,
  override.cpp entry copies, cbshim_report keys, ACF 93 text. T1 launch scripts give both arms identical conditions
  except tm_soft; a refused or mismatched parameter stops the run before play; the logging patch does not change
  play; report arithmetic correct on complete inputs.
- Accepted: TranspositionTable::resize(mb) wraps for mb >= 2^44 when called directly (the option takes an int).
- r10k4 runs recomputed from records: KingsRow mode 3 1/310/1 and 0/312/0, Cake 1/310/1, GuiNN 16/296/0; no lost
  game met a changed book position.

## Round 20 (2026-11-11)

- Handed to the search-lab owner: experiments/search-lab/post.sh goes on to append, copy, commit and push when
  row.py stops with an error (empty or missing log).
- Verified: selfplay gives both sides identical conditions except the named parameter; pair accounting cannot count
  a pair twice or half; Elo and interval follow from the pentanomial; --skip indexes the shuffled list (search-lab
  branch only). Book Lab's round-19 fixes pass. T1 repeat a recomputed from records and per-move logs: Cake
  +2.5 [-0.5, +6.0], KingsRow mode 3 -1.0 [-2.5, 0.0]; rule keeps tm_soft 60. Pin list wp11n/pins_r11n (41,599
  pins on r10k5, book r11n e8010b13...) passes legality, source, loss and base-book checks.
- Known and accepted: selfplay draws after 80 reversible plies, the match runner and the search after 79.

## Reviewer B round 12 (2026-11-11)

- Handed to Book Lab: wp12_discover.py on resume treats ledger rows as done before checking their manifest (a changed
  runner, host or baseline is not noticed for completed rows); cbshim_report.py still counts the launch warm-up for
  a game that starts from the standard position without ballot moves.
- Thread-checker pass (Engine and CheckerBoard entry points, 2 and 4 workers, database off and on): no diagnostic
  from the hash table or the search; the two interface findings are fixed in PR #60. The plain-int stop flag stays
  the accepted interface.
- NNUE read complete (accumulator updates, buckets, scalar/NEON by execution, AVX2/VNNI by a portable model,
  output scaling): no other disagreement. No book entry of r10 or r10k lies at 8 or fewer pieces.
- Search opinions (not defects): repairs 4-8 (#62) and 2-7 (#93) are supported by 20M and 100M searches of both nets;
  the #55 loss shows no early turning point (first shared preference at ply 36, gain 2-7); #53: mixed.
- hash_resize_refused needs allocator_may_return_null under the address checker (set in CMake, PR #60).

## Reviewer B round 13 (2026-11-11)

- Handed to Book Lab: pinwalk.cpp / pins.py / lost_vs_book.py do not see an avoid entry of the base book (a move the
  base book excludes can be pinned, and booklab_apply then refuses the list; a loss made unplayable by an avoid
  entry is not treated as repaired). The committed r11c list is not affected (a reference with exclusions gives
  the same 45,916 pins).
- Accepted: the net loader's range check is conservative (it can refuse a net that is safe because one side has at
  most 12 pieces); it cannot accept an unsafe one.
- Verified: PR #60 interface cases clean under the thread checker; r11c pin list rebuilt byte for byte; r11c n6c
  validation recounted and replayed: 22 wins, 1,538 draws, no loss in 1,560 games.
- Analysis recorded (not defects): our win against Cake (ACF 163, Cake Black): Cake's first clearly worse move is
  ply 51 (26-30 instead of 24-27, 13 pieces); none of Cake's six other games of that opening reaches that position,
  so the win is not a replayable line. n9h / n8q / n6c opinion table on the lost-game positions is in the report.

## External deep analysis (GPT-6 Pro, 2026-11-11) - checked by the lead

- Fixed on master: table move ignoring game history (PR #65).
- Confirmed and handed to the truth-data owner: `truth relabel` writes the first covered position's database value
  on every record of the game.
- Under test, not defects: shallow pruning of quiet moves that force a capture (search-lab experiment Q); Cake's
  move choice depending on the state of its process (Book Lab, --fresh-engines).
- H3 is no holdout for books from r11b on (its Cake games are in the pin pool); panel H4 added (PR #67).

## Round 21 (reviewer A, last round on the first machine, 2026-11-11)

- Handed to Book Lab (win-mining tools at booklab fa0c887): wp12_select.py per-opening limit ignores contexts from
  another validation repeat; the known point bypasses the eligibility checks (protected position, missing score,
  move already played by the base book -> "None"); wp12_discover.py counts a searched move as book exposure and
  does not know the termination text "lost on time"; lost_vs_book.py / wins_vs_book.py report a failed replay as a
  clean result and wins_vs_book.py drops the FEN tag; wp12_chain.sh freezes candidate contexts before the
  validation runs are complete (20 points of the committed c1 files miss a context).
- Recorded gap, not a defect: stages 2 to 4 of experiments/booklab/wp12/PROTOCOL.md (confirmation, other contexts,
  whole-book acceptance) have no runner and no evaluator; nothing can be accepted beyond a discovery nomination.
- Verified: all ten full r11c validation runs recounted and replayed (3,120 games, no loss); pin use per game;
  PR #59; search-lab post.sh fix; discovery runtime-manifest check.

## Where the reviewers run (from 2026-11-11 08:15)

Nothing of the review runs on the lead's machine any more. Reviewers are started on the Windows machine with their
own clone under `~/regicide-review-*`, lowest priority, at most 2 threads, and write their reports there.

## Reviewer B round 14 (last round on the first machine, closed early, 2026-11-11)

- Accepted (do not re-report): if the copy of the option values cannot be made after a successful `set` (allocation
  failure inside publish), `get` reports the previous value until the next successful `set`. PR #61 keeps the
  process alive in that case; the stale read-back under memory exhaustion is accepted.
- Handed to Book Lab: pins.py / lost_vs_book.py count a later avoided own move as a downstream repair only at
  book-eligible positions, although the engine applies the avoid list inside a run of king moves too;
  wins_vs_book.py (failed replay certified, FEN tag dropped; also in round 21).
- Verified: the built-in book r11c on master is byte-identical to the validated file; all 81,061 entries and five
  avoid entries are legal; no position has two forced moves; through the engine 2,400 sampled pins are played at
  depth 0 with zero nodes; king-run positions and database-known roots search instead.
- Analysis (opinions of our search, not proofs; master 85a9fe8, n6c, 7-piece database): KingsRow's first clearly
  worse move in our won games: ACF 43 ply 23 8-12 (8-11), ACF 71 ply 25 10-14 or 13-17 (5-9), ACF 134 ply 37 20-24
  (11-16); Cake ACF 163 ply 51 26-30 (24-27). 24 of 67 won games analysed; the rest (H3 panel games against Cake)
  is not pursued. Hand-over files: branch t3/review-handover, experiments/review-handover/b14/.

## Reviewers on the Windows machine, rounds A1 to A3 and B1 (2026-11-11)

Reports: branch `t3/review-reports`, `reviews/a/roundN/` and `reviews/b/roundN/`.

- Fixed on master: PR #71 (two findings of A1, see the table above). PR #73 (`--pairs-together`) checked in A3:
  no defect.
- Accepted: stale option read-back after an allocation failure during publication (A1; same as reviewer B
  round 14).
- Handed to Book Lab and fixed there (33e8910): wp12_chain.sh completeness guard accepted a short run with
  exit_status 0; wp12_confirm.py and wp12_discover.py loaded engines before taking the timing lock (A2).
- Handed to the search-lab owner: prune statistics kept across a `go` answered from the tables (A2).
- Settled, not defects: `Book::probe` at margin 0 (book mode 3) always returns one fixed move, score ties included;
  the seed is not used there. Our own play inside one configuration: book moves never differ between games,
  searched moves differ in 9 of 17,083 comparable pairs (A3). The plain-int stop flag and the time-based search
  remain the known sources of difference.
- Verified: PR #60, #61, #65, #66, #71; `truth relabel` / `labelcheck` fix on truth-data (B1); the two hand-over
  items (avoided move inside a king run in pins.py; wins_vs_book.py replay and FEN) are fixed on booklab (B1).
- Analysis (opinions of our search with n8q and the 8-piece database, B1): KingsRow's moves 9-14 (ACF 16 ply 41),
  8-12 (ACF 43 ply 23), 10-14 / 13-17 (ACF 71 ply 25) and 20-24 (ACF 134 ply 37) are worse than the alternative by
  94 to 226 hundredths at 100M nodes; Cake's 20-24 and 10-15 on ACF 163 do not pass the 30 threshold.


## Reviewers on the Windows machine, rounds A4 to A6 and B2 (2026-11-11)

- Fixed on master: PR #75 (regression test for the per-engine game counters after a restart), PR #76 (A5: the
  runner ignored and did not record engine claims; now always recorded, `--cb-claims` and `--cb-move-limit` follow
  a CheckerBoard engine match), PR #77 (A5: our DLL conceded a known loss; under CheckerBoard's early adjudication
  that ends the game at once). A6 checked #77: complete, its test fails on the code before it.
- Handed to Book Lab and fixed there (b9a0f35): wp12_confirm.py combined fresh processes with pairs (A4).
- Open, assigned to the integrator (branch t3/claim-soundness), all four checked against the code (A6):
  1. a table win is claimed although the no-progress limit arrives before the conversion (B:WK3:BK1,K2 after 77
     reversible plies: 1-5, score 19994, CB_WIN; the game is a draw);
  2. a search win is claimed at a root that is already a third occurrence; and no draw is claimed when our own move
     completes a rule draw, which CheckerBoard's detector can miss at its window boundary;
  3. `--cb-move-limit N`: a claim made by search N+1 is scored, CheckerBoard records unknown;
  4. `--cb-claims` with `--accept-draw-claims` turns a capture that leaves the opponent without pieces into a draw.
- Accepted differences to CheckerBoard (A5): the repetition window boundary differs by one ply, one 0.1 s warm-up
  search per engine is not reproduced, time-out policy. Base profile for a comparable match: concurrency 1,
  full deck in order, pairs together, processes kept, `--cb-claims --cb-move-limit 200`.
- Under reading (B3): with the stop already requested before the call, the fallback move at a table-drawn root
  can leave the draw (B:WK30:BK25,K26: 25-21). Not reachable in an engine match (no stop before the call).
- Settled, not defects (A6): static and conversion scores cannot reach the win-claim threshold; a stopped child
  never writes a root score; the built-in book r11d has score range -650..191, no book-derived claim possible.
- Records (A6, recount of Book Lab e2c1655): the three concurrency-1 runs on 1c6e997 are Cake 0/312/0,
  KingsRow 1/310/1 (loss: ACF 6 as Black, game 9, both processes 8 games old), GuiNN 21/291/0; both process
  counters equal game number minus one in all 936 games. All 6 fresh and 6 kept Cake decks: 3,744 draws.
- Analysis (B2; opinions of our search, n8q, 8-piece database): in the 20 drawn games with our highest scores no
  missed win of ours and no opponent error before it; mining list entries 1 to 12: besides the known ACF 71 point,
  GuiNN's 16-20 and 4-8 on ACF 141 ply 13 are worse than 11-15 by 130 at 100M nodes.
