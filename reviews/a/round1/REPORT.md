# Regicide review — round 1

Review directory: /home/admingod/regicide-review-a  
Report: /home/admingod/regicide-review-a/reports/round1/REPORT.md  
Reviewer: review sub-agent on Win11-AdminGod, WSL Ubuntu-24.04, user admingod.  
Date: 2026-11-11.

## Revision, ledger and scope

I read `docs/REVIEW_LEDGER.md` from `origin/master` before reviewing. The initial fetched master was c1c50bc19c92eeb9a313af0bca681d33d6ad3583. The final fetch advanced it to **4a3ad5278ad6115c13f828eac56109ccc7a1f11c** (PR #69, ledger only). I read the additions, checked out that revision, verified that src/, tools/ and tests/ are unchanged, and repeated the current regression/root-policy checks. None of the findings below is already assigned or accepted in that ledger. Findings 1–3 describe gaps in the new fixes, rather than repeating the problems the ledger lists as fixed.

The final fetched booklab revision was **0ae8af2c700e557431e164f7a2bd0bfa3b716633**. The two requested compressed game files were unchanged from the initial booklab fetch (fa0c887).

I made the clone here using the remote URL read from ~/regicide-booklab. That was the only access to another agent's checkout. I did not use the shared endgame database, change any other checkout, touch a lock, signal another process, commit or push. All test fixtures, temporary files and outputs are under this review directory. Build/test/engine invocations used `nice -n 19`; the build used `-j2`, searches used one worker, and the concurrency test used two threads. Each command finished in seconds. The runner tests used scripted protocol fixtures, not games against another checkers engine.

## Findings

### Finding 1 — Restricted analysis returns a table loss for an immediate draw

**File/line:** src/search.cpp:810–813, especially the `!analysis` condition at line 811.  
**Priority:** P2.  
**Fix being reviewed:** PR #65, 691242c.

Concrete legal input, using the generated 2–3-piece WLD/MTC database:

1. Start at `B:WK21,K25:BK4`.
2. Play `4-8 25-30 8-11 21-17 11-15 17-13 15-19 13-17 19-15 17-21 15-10 21-17`.
3. Pass every position before its move as the chronological history to `set_position`.
4. The root is `B:WK17,K30:BK10`. Set egdb_pieces=3, egdb_mtc_pieces=3, depth=4, and searchmoves={10-15}; leave infinite=false.

The result is `best=10-15, score=-19973, nodes=0, depth=0`. The child `W:WK17,K30:BK15` already occurs twice within the reversible history. This move draws immediately, so the score for this restricted line must be 0. The unrestricted call returns 10-15 with score 0; the same restricted call with infinite=true returns score 0 after 8 nodes; disabling MTC also returns 0.

The same gap applies to the no-progress limit. The legal 78-ply quiet history in root_review.cpp/root-review.log reaches `B:WK22,K31:BK5`. Restricting analysis to 5-1 returns `score=-19971, nodes=0`, although that move reaches reversible ply 79 and draws. The unrestricted call returns score 0.

The draw override is disabled for restricted analysis, but the earlier table shortcut is still enabled for it. A restricted list must still determine the value of its moves using game history. The override can run over that list, or the table answer can be discarded and the restricted search allowed to score the child.

**Ran:** reports/round1/root_review.cpp; output in root-review.log. These examples use the current production Engine::go and the small generated database.

### Finding 2 — EngineGames loses the surviving process's game count after a restart

**File/line:** tools/match/main.cpp:599–600 and 620–621.  
**Priority:** P2.  
**Fix being reviewed:** PR #66, 2028e94.

Concrete input: one standard-start opening, `--rounds 2 --max-plies 2 -c 1`, default engine reuse; two scripted protocol fixtures. Fixture A exits on its first search of the second real game; B stays alive. The exact invocation and fixture are reproducible from fake_engine.py and the command below.

The PDNs contain:

| Round | Termination | Worker | EngineGames |
|---|---|---:|---:|
| 1.1 | max plies | 0 | 0 |
| 1.2 | A crashed | 0 | 1 |
| 2.1 | max plies | 0 | **0** |
| 2.2 | A crashed | 0 | 1 |

At round 2.1, A has been replaced and has played zero preceding real games. B is still the original process and has participated in both preceding games (including the crash game). The tag nevertheless says 0. The PID log proves the lifetime difference: A changes from PID 438879 to 438881; B remains PID 438880. Even if only completed normal games were counted, B has already played round 1.1.

Resetting one shared counter whenever either engine restarts cannot describe how many games both current engine processes have played. It makes a reused opponent look fresh, defeating this tag's purpose when studying process state. Keep counts per engine (and label them by engine/colour), or replace both processes together whenever the shared count is reset.

**Ran:** pid-crash4/games.pdn, crash4-pids.jsonl and pid-crash4.log. The fixture's launch/search/newgame log is independent of the PDN tag. This reports a metadata error; it does not claim that the restart changes the scored result of these fixture games.

### Finding 3 — A successful option change can leave get reporting the previous value

**File/line:** src/cb_api.cpp:89–95, with the publishing destructor after `set`.  
**Priority:** P3 (allocation-failure path).  
**Fix being reviewed:** PR #61, 2374e46, following PR #60.

Concrete input:

1. Through enginecommand, successfully load the repository's valid eval.bin from the long path created under reports/round1/tmp/ (an 80-character directory name). This ensures the published evalfile string needs an allocation.
2. Start with book=3 and a matching published snapshot.
3. Inject allocation failure, then call `enginecommand("set book 0", reply)`. This particular short command and setting reach publication without needing a preceding allocation.
4. Restore allocation and call `enginecommand("get book", reply)`.

Observed output:

```
set book 0 ret=1 reply=0 actual=0 get book=3
```

The setter acknowledges success and disables the engine's book, but the copy of the unrelated long evalfile path fails inside publish(). The catch keeps the entire old snapshot. A later query reports book=3 although the engine is using book=0; queries do not retry publication, so this lasts until another successful publish.

The new function does meet the requirement that publishing never throws, and the refused-setting crash is fixed. Its failure handling also needs to cover a setting that has already succeeded. In this example the scalar shown book field could be updated without allocating; another approach is to prepare the publishable state before changing the engine.

**Ran:** publish.cpp, mode `stale`, linked to current core and including current cb_api.cpp; output publish-stale.log. The harness overrides operator new only for the injected interval. The long evaluation path was installed through the real enginecommand API; the setup does not assign the option fields directly. The `actual` diagnostic reads the private live book_mode solely to confirm the state; the contradictory return values are both public API responses.

## Recent changes and regression-test checks

### PR #65

The committed egdb_root test passes on current master. I compiled that same current test with the parent-of-691242c version of src/search.cpp, linked against the otherwise current core. It fails with:

```
FAIL: a lost root takes the draw by a third occurrence
```

The code outside this translation unit is not being claimed as a full historical build. The current test detects this specific before/after change. Its added case covers an unrestricted MTC loss taking a third occurrence and the same board without history choosing the table defence. It does not cover restricted analysis, no-progress boundaries, opposite-side keys, or the won-root fallback; those were exercised separately here.

Window and boundary results from root_review.cpp:

| Case | Result |
|---|---|
| Two earlier child occurrences in the reversible stretch | 10-15, score 0 |
| First required occurrence is the oldest position still in the window | 10-15, score 0 |
| Trim one more earlier position, leaving one occurrence | table defence 10-6, score -19973 |
| Replace the two matching boards' side to move by the other side | table defence 10-6, score -19973 |
| Put the matching keys before a man/piece-state boundary | table answer, no false draw |
| Legal history with 77 preceding quiet plies | no premature no-progress draw |
| Legal history with 78 preceding quiet plies, lost root | score 0 on the next quiet move |
| Won root with 78 preceding quiet plies | legal searched move, score 0, 28 nodes |
| Every candidate child marked as a third occurrence | legal searched move, score 0, 12 nodes |
| Quiet move leaves the opponent with no legal move at the limit | 6-2 from B:W5:BK1,K6, positive table win |

The opposite-side, history-boundary, all-candidate-repetition and terminal-at-limit fixtures deliberately construct histories to isolate the key/window/rule boundaries; they are not asserted to be complete reachable games. The main repetition reproduction and the 78-ply quiet walks are legal replays. Full histories and calls are retained in root_review.cpp/root-review.log.

The window calculation is correct: for a reversible child with count R, the root is one of those earlier R positions and has the other side to move. Therefore at most R-1 positions before the root are relevant. Taking the final min(history size, R-1) entries is the right interval. Position keys include the side to move. Captures and man moves reset the reversible count; child count 79 triggers the no-progress rule. Checking for no replies before either draw rule correctly gives a terminal win priority.

I found no path by which rejecting the won root's table move alone leaves it without a move. It falls through to the search; the winning-move filter replaces the list only when nonempty. The fallback test with every child drawing still returns a legal move. The existing table move helper returns nullopt if its candidate list has no acceptable winning child, which also reaches the search.

### PR #66

The committed `runner_test fresh` passes on current master. I built the parent-of-2028e94 main.cpp with the current, unchanged runner support files and ran the current test against it. It fails: the default run has no expected tags, and --fresh-engines is refused. There are no changes between that parent and current master in engine_process.cpp, openings.cpp, stats.cpp or tools/common/.

I additionally checked actual process lifetimes using independent fixture PID logs over four real fixture games:

| Mode | Launches per engine | newgame messages per engine | PDN EngineGames |
|---|---:|---:|---|
| default reuse | 1 | 5 (warm-up plus 4 games) | 0, 1, 2, 3 |
| --fresh-engines | 4 | 8 (warm-up plus game for each launch) | 0, 0, 0, 0 |

Thus the option really replaces the processes in the normal path. Worker=0 is correct for the one-worker cases. Worker IDs are worker identities, not chronological round assignments or PIDs.

The committed test checks tags, not process identity. As a coverage check, I made a copy of main.cpp under reports/round1/ with only the fresh-mode kill call removed, leaving its counter reset. The committed fresh test still exits 0 (tag-only-test.log). This is a test coverage limitation, not a claim that production fresh mode fails to relaunch. Finding 2 is the separately reproduced production gap on asymmetric restart.

### PR #60 and #61

Neither change added a dedicated committed interface-concurrency/publication regression test. PR #60 added NNUE boundary assertions and the allocator environment for a different test; PR #61 changed only cb_api.cpp. The existing cb_api test passes, but its serial command/search cases do not establish these concurrent or allocation-failure guarantees.

I ran a custom two-thread ThreadSanitizer harness for concurrent first calls, then a separate mode that initializes first and races get book against set book:

| Check | Current cb_api.cpp | Code before PR #60 |
|---|---|---|
| concurrent first calls | exit 0, errors=0, no TSAN warning | exit 66, engine-pointer initialization race |
| initialized query/write loop | exit 0, errors=0, no TSAN warning | exit 66, book_mode read/write race |

Each thread makes 10,000 calls. cb_api.cpp and the harness were instrumented; the linked core library was not rebuilt with TSAN. These results validate the interface synchronization being reviewed, not all code in the engine. Source inspection confirms that call_once encloses complete initialization; setters hold mutex, query copies hold shown_mutex, and there is no reverse lock order.

I also injected allocation failure while publishing a long-path snapshot from the same noexcept destructor pattern as the setter. Current code survives and retains the previous copy. With cb_api.cpp from before PR #61, the harness terminates with std::bad_alloc, exit 134. Current publish() contains the copy and swap inside catch-all handling and is declared noexcept; moving/swapping the ordinary string members does not allocate. This fixes the original destructor exception. Finding 3 is the remaining successful-setting case.

## Engine::go rule precedence

I read the complete function and the relevant Worker setup, repetition, table-probe, root-storage and iteration logic. The following partitions cover the combinations of the named root policies; the 192-case executable tests representative positions, not every possible board or history.

| Root / call condition | Rule that takes priority | Assessment |
|---|---|---|
| No legal moves | Return before root policy | Correct; no move is possible |
| Nonempty legal searchmoves | Intersect before table lookup; bypass book/avoid; bypass general won-root filter | Correct subset handling; finding 1 affects history-adjusted loss values |
| Database unknown, ordinary play | Avoid filter, then eligible book probe, then search | Correct ordering; avoid falls back to all if none remain |
| Database unknown during king-move stretch | Book skipped; avoid still applies | Correct |
| Database draw | Neither book nor avoid; ordinary search uses child WLD values | Correct in tested cases; no MTC draw shortcut |
| Database win/loss, finite call, usable MTC | Table move among the selected root moves | Correct except finding 1 |
| Database loss, finite unrestricted call, immediate draw available | Immediate draw overrides table defence, even if no MTC answer exists | Correct |
| Database win, chosen table move immediately draws | Discard table answer, continue into search | Correct fallback; terminal winning moves retain priority |
| Database win, no table answer, unrestricted | Keep WLD-winning/terminal children if any; then conversion search | Correct; filter cannot empty the list |
| Database win/loss without usable MTC | Conversion search, including the game draw rules | Correct in tested cases |
| Any infinite call | No table return and no book; search runs under supplied depth/node/time limits | Correct; avoids still apply only at unknown unrestricted roots |
| Infinite database win, unrestricted | WLD-winning subset retained, search runs | Consistent with won-root policy |
| Infinite or finite restricted analysis | Avoid and book skipped; full supplied search limits for a single move unless finite MTC answers it | Correct apart from finding 1 |
| Book and avoid both name a move | Book probe excludes avoided moves; search uses remaining candidates if no book answer | Correct |
| Book/avoid at any covered root, including a capture | Database policy wins | Correct |

The executable policy_matrix.cpp runs 192 combinations: won 3-piece root, lost 3-piece root, drawn 2-piece root, uncovered 4-piece root; MTC off/on; finite/infinite; unrestricted/first-move-only/last-move-only; book off/on; avoid off/on. It checks legality, membership in a requested subset, and preservation of an unrestricted root's WLD win/draw. Output: `matrix cases=192 failures=0`. History-sensitive cases are in the separate root harness, including the reported gap.

An all-illegal searchmoves list falls back to all legal moves in the implementation. The interface documents searchmoves as legal moves; I did not count invalid caller input as a defect. Existing accepted approximations about deeper history-dependent hash results and database wins beyond the no-progress horizon are not re-reported.

## Record check: known163 against the earlier reference game

I extracted the requested gzip files directly from origin/t3/booklab into reports/round1. records.py separates tags, main-line moves and brace comments. It replays every main-line move in all 20 known163 games and all 156 games of bl-v13k4-cake-a-h2 through the independent tools/common referee using replay.cpp; any illegal/ambiguous move stops with an error. All replayed successfully. Both sides to move and king flags are included in each compared FEN. Comment depth/node fields were attached to the actual preceding main-line move, not to PV text.

The reference is uniquely round **69.2**, Cake Black / Regicide White, result **0-1**, opening 12-16 21-17 16-20, termination black has no legal moves.

### Statements checked

1. **Twenty games of the same opening, Cake Black and Regicide White:** true. The tags identify the same opening and 0.5s/move CheckerBoard match rules throughout.
2. **All six Regicide wins lie in rounds 9–16:** true. Exactly rounds **9.2, 10.2, 11.2, 12.2, 14.2, 15.2** are 0-1. Rounds **13.2 and 16.2 are drawn**, as are all rounds 1–8 and 17–20. If “exactly the games of rounds 9 to 16” means every one of those eight games was won, that reading is false.
3. **Rounds 1–8: ply 15 is 4-8, depth 20, 6,280,939 nodes each:** true, all eight.
4. **Rounds 9–16: ply 15 is 10-15, depth 25, 13,976,870 nodes each:** true, all eight, including the two draws.
5. **Rounds 17–20: ply 15 is 4-8, depth 26 or 27:** true. Exact values follow below.
6. **All six won games transpose into reference round 69.2 before ply 30:** true. Their first return to the reference after the differing move order is **after ply 17**. The reference plays 4-8, 25-22, 10-15 at plies 15–17; these six play 10-15, 25-22, 4-8. The resulting board and side to move match. This is also true of drawn rounds 13.2 and 16.2; the transposition is not unique to the wins.
7. **At B:W9,12,15,21,25,26,32:B1,2,3,14,16,20, Cake plays 20-24 six times, all lost, and 14-18 three times, all drawn:** true **within the 20 known163 games**. It occurs before ply 37. The 20-24 games are rounds 9, 10, 11, 12, 14, 15; the 14-18 games are rounds 13, 16, 17. The other eleven games do not reach this position. If reference round 69.2 is added to the population, it contributes another 20-24 loss: seven, not six.
8. **The matches ran eight at a time with reused processes:** this is supplied run context, not something these two PDNs establish. They have no worker, process, engine-lifetime or concurrency tags.

### Per-round checks

Results below are from Regicide's viewpoint. All quoted node counts are exactly the integers in Cake's comment at ply 15.

| Round | Result | Cake ply 15 | Depth | Nodes | Cake at target, ply 37 |
|---:|---|---|---:|---:|---|
| 1 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 2 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 3 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 4 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 5 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 6 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 7 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 8 | draw | 4-8 | 20 | 6,280,939 | not reached |
| 9 | win | 10-15 | 25 | 13,976,870 | 20-24 |
| 10 | win | 10-15 | 25 | 13,976,870 | 20-24 |
| 11 | win | 10-15 | 25 | 13,976,870 | 20-24 |
| 12 | win | 10-15 | 25 | 13,976,870 | 20-24 |
| 13 | draw | 10-15 | 25 | 13,976,870 | 14-18 |
| 14 | win | 10-15 | 25 | 13,976,870 | 20-24 |
| 15 | win | 10-15 | 25 | 13,976,870 | 20-24 |
| 16 | draw | 10-15 | 25 | 13,976,870 | 14-18 |
| 17 | draw | 4-8 | 26 | 16,823,238 | 14-18 |
| 18 | draw | 4-8 | 26 | 16,840,874 | not reached |
| 19 | draw | 4-8 | 27 | 23,265,359 | not reached |
| 20 | draw | 4-8 | 27 | 23,330,867 | not reached |

Rounds 1–8 and 17, 18, 20 also share the reference board after ply 29 despite later draws; round 19 has no return after its first move-order difference. records.json contains every compared position and attached comment, and record-check.log retains the match indices.

### What these records cannot establish

The PDNs have only Date (same day), Round, player/opening/time-control/result/termination metadata, and move comments. They do not identify which worker played any game, which rounds shared a process, startup/restart events, exact start/finish times, or internal engine state. Aggregate file order is completion order in this runner, and the known163 records are not ordered by round. Even given the supplied concurrency of eight, rounds 9–16 cannot be assigned one-to-one to particular workers or process ages from this file.

Repeated depth/node totals and move choices are observed facts. They do not by themselves prove which hash/history/book state caused the groups, whether Cake retained any particular table, or that a fresh-process run would reproduce or eliminate them. The 20-24/14-18 result association is a statement about these played continuations, not an independent proof of either move's game-theoretic value. The Date tags alone also cannot establish which same-day run happened earlier.

## State between games

### Runner and host messages

Read: tools/match/main.cpp, tools/match/engine_process.cpp, tools/cbhost/cbhost.cpp and src/cb_api.cpp.

By default, each match worker retains its own two engine processes. At launch/relaunch, Engine::launch:

- loads the host/engine and waits for ready;
- sends configured `cmd ...` options;
- sends `newgame`;
- sends `position startpos`;
- performs the warm-up `go maxtime 0.05 reset`.

At each real game, play_game sends `newgame` to each host and waits for ok. On each side's first search it sends a full `position ... [moves ...]`, waits for ok, then sends `go maxtime ... reset` plus exact/incremental time arguments when applicable. Later searches omit reset. SideClock is a new local object for each game, so the runner's bank is reset.

The host's newgame handler resets its Game and sets reset_pending. It does **not** call an enginecommand named newgame. The engine DLL receives the new-game notification through **CB_RESET_MOVES on its first getmove call**. A go reset also sets that pending flag. By default cbhost sends `set gamehist <FEN> <moves...>` before each getmove; --no-gamehist disables that command. After getmove, reset_pending is cleared.

With --fresh-engines both processes are killed and relaunched before each subsequent game, including options and the warm-up above. Thus “fresh” still includes one warm-up search in each process before its game. That is deliberate initialization, not a zero-search process.

### What Regicide clears and retains

| State | Behaviour on the reset flag / following search |
|---|---|
| Transposition table and generation | Engine::new_game clears the table and resets generation |
| Quiet history and continuation history | Worker::clear zeros them |
| Killer moves | Worker::clear zeros them |
| Game history supplied by cbhost | If matching, replaces prior history; otherwise reset clears inferred history |
| Pending supplied history | gamehist is consumed and cleared |
| Inferred prior-game after-position | have_after can remain stored until a search, but reset prevents it from extending prior-game history; then it is replaced |
| Engine root/history keys | set_position replaces them on the next getmove |
| Per-search root list, nodes, table hits, depth, partial score, NNUE accumulator | rebuilt/reset by Worker::setup when search runs |
| conversion mode, stop flag, LMR table | set again by go |
| Hash size, worker count, evaluator, book mode/book data, avoid data and search parameters | retained configuration |
| Endgame mapping/cache | retained process-wide; no per-game unload |
| Engine book random seed | **retained**; new_game does not reset book_seed_ |
| Time bank inside Regicide's CB adapter | none exists; the supplied maxtime or incremental remaining/inc values drive each call |

The retained **book random seed can change later play**. It advances on eligible book probes, including probes that miss. A core harness with seven equal-score legal starting-book moves, margin 60, and new_game before each call returns:

```
11-15 12-16 9-13 12-16 9-13 10-14 9-13 11-15
```

This is retained state that can affect modes 1 and 2 when several eligible moves exist. Mode 3 sets margin=0, and Book::probe then chooses only its first sorted entry (n=1), so the seed does not change that mode's selected move. The supplied built-in-book match configuration uses mode 3; no carry-over of Regicide's search hash/history is established by these records. Configuration and loaded evaluation/book/database data also affect play but are intentionally kept settings.

The public core Engine::new_game alone does not erase root_ or history_; its clients set_position for a new game. The reviewed CB adapter does so on every getmove and resets/overwrites game history as above. I found no prior-game history leak through that adapter with a reset flag. I did not inspect Cake's implementation, and this report does not infer that Cake responds to CB_RESET_MOVES in the same way.

## Commands, artifacts and limits of verification

Build:

```sh
cd /home/admingod/regicide-review-a
nice -n 19 cmake -S . -B build-review -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DREGICIDE_LTO=OFF
nice -n 19 cmake --build build-review -j2 --target egdb_root_test cb_api_test regicide_cb runner_test match cbhost
TMPDIR=$PWD/reports/round1/tmp nice -n 19 ctest --test-dir build-review -R '^(egdb_root|cb_api|match_fresh_engines)$' -j1 --output-on-failure
```

The three selected committed tests pass. I did not run the full ctest suite, a full-engine sanitizer build, Windows DLL execution, timed real-engine matches or the shared 2–8-piece database. Compilation and direct executions here use Linux under WSL. Windows-only behaviour was read, not executed.

Custom executable sources and primary output files (all under reports/round1/):

- root_review.cpp / root-review.log: legal repetition reproduction, restricted analysis, no-progress, boundary fixtures and fallback.
- policy_matrix.cpp / policy-matrix.log: 192 policy combinations and retained book-seed demonstration.
- cb_concurrency.cpp / cb-current-tsan.log / cb-current-getset-tsan.log: current interface checks; cb-before60-tsan.log and cb-before60-getset-tsan.log: historical failing comparisons.
- publish.cpp / publish-current.log / publish-before61.log / publish-stale.log: nonthrowing publication and successful-setting stale snapshot.
- fake_engine.py / kept4-pids.jsonl / fresh4-pids.jsonl / crash4-pids.jsonl: independent launch/newgame/search evidence.
- pid-kept4/, pid-fresh4/, pid-crash4/: runner PDNs, logs and summaries.
- records.py / replay.cpp / records.json / record-check.log: all main-line replay and comment extraction.
- before65.log, before66.log, tag-only-test.log: historical and mutation regression checks.
- configure.log, build.log, targeted-ctest.log, final-ctest.log and ledger-final.txt: build/test/ledger evidence.

Runner restart reproduction:

```sh
nice -n 19 build-review/tools/match \
 -a "exec:/usr/bin/python3 $PWD/reports/round1/fake_engine.py A crash $PWD/reports/round1/crash4-pids.jsonl" \
 -b "exec:/usr/bin/python3 $PWD/reports/round1/fake_engine.py B keep $PWD/reports/round1/crash4-pids.jsonl" \
 --openings reports/round1/opening.txt --max-plies 2 --rounds 2 -c 1 \
 -o reports/round1/pid-crash4
```

For a new reproduction output, use a new filename/directory so PID logs are not appended to the saved evidence. All saved main code and committed tests remain unchanged; only reports/ is untracked.

