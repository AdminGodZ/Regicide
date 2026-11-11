# Review A — round 2

Date: 2026-11-11. Repository: AdminGodZ/Regicide.

Round 1 was published first, as requested: reviews/a/round1/REPORT.md, commit cf8338db11f7327c58bbaa1ca02de50eadadf679 on t3/review-reports.

## Revisions and method

I fetched before starting and again before reporting, and read the master review ledger both times. Final reviewed heads:

| Ref | Commit |
|---|---|
| origin/master | 1c6e99715299a5fbbbd407b1b9361e3c299e91e6 |
| origin/t3/root-analysis-tags | 121d097acb20e5fc15fa358b94c4d8c1b64e1dc8 (merged into master by PR #71) |
| origin/t3/booklab | 1a1b6541a747dda2ec9205688fb46459a2504dc1 |
| origin/t3/search-lab | d4370a3cb3fbc762bb9ece6720c01f81a86344ff |

Master advanced during the review: it merged the two round-1 fixes, replaced r11c with r11d in PR #72, and added --pairs-together in PR #73. I retained the specifically requested r11c measurement at f032e23 and additionally counted the new master book. The relevant Book::probe, CB adapter and TT code did not change. The search.cpp change on master is exactly the restricted-analysis fix tested here.

The accepted stale option read-back under allocation failure is not reported again. The old avoid-in-king-runs report was also left with its owner; the newest Book Lab changes to those scripts do not invalidate the replay-error tests below.

Work stayed inside /home/admingod/regicide-review-a. Remote branches were read through git and extracted into private source directories for tests; only t3/review-reports was checked out/committed/pushed. No master or feature branch was edited. No other agent's directories, processes or lock files were accessed. I did not run the production launch scripts or any real-engine matches. The launcher-order test replaces engine loading, process creation and locking with in-memory mocks; it opens no lock file. All builds/searches/tests used nice -n 19, at most two build jobs, and one search worker (two threads only where a protocol harness inherently has its main thread and one worker). The generated 2–3-piece database stayed under the review directory.

## Findings

### 1. The validation-context guard accepts a short run with exit_status 0

**File:** experiments/booklab/scripts/wp12_chain.sh:21 on Book Lab 1a1b654.  
**Priority:** P2.  
**Fix being checked:** 0ae8af2, Round-21 finding 6.

The new guard is:

```sh
[ "$(cat $d/exit_status 2>/dev/null)" = 0 ] ||
  { [ ! -f $d/exit_status ] &&
    grep -Eq "^Games: ([0-9]+) of \\1 scheduled" $d/summary.txt; }
```

Concrete input: a context run directory containing:

```
exit_status: 0
summary.txt: Games: 1 of 156 scheduled
```

The actual predicate prints **ACCEPTED 1 of 156**. It never reads the summary when exit_status is 0. After the V11C done marker, this directory can therefore enter --context-runs and freeze incomplete contexts.

This distinction matters in this repository: bl_run_w.sh writes the match process's rc to exit_status before its separate completion check changes the wrapper's return code to 4 for an incomplete run. Thus an incomplete directory can have exit_status=0 while the wrapper itself failed. The V11C marker also does not certify the historical bl-v13k4, bl-v13-kingsrow, bl-k3-kingsrow and bl-f1 context families enumerated here.

Require a successful status *and* the appropriate complete-game check for runs with a marker, while retaining the summary-only check for the explicitly supported older runs. This is a residual in the new all-contexts completion fix, not a repeat of the original early-start report.

**Ran:** the exact new shell predicate against the two text files above in reports/round2/short-context/. Output is retained in context-completion-review.log. I did not start the chain.

### 2. The new confirmation runner loads engines before taking the timing lock

**File:** tools/booklab/wp12_confirm.py:153–160 on Book Lab 1a1b654.  
**Priority:** P2.  
**Introduced by:** 0ae8af2.

Concrete call: stage2 for three blocks, four candidate and four control games per block, with an incomplete first block so the runner must start it.

The three readback() calls at lines 153–154 invoke book_probe.py, which starts cbhost and loads the candidate Regicide, control Regicide and Cake DLLs. Only afterward does the runner open LOCK and acquire LOCK_EX at lines 159–160. Regicide's DLL initialization maps/initializes its database, as already established in the earlier wrapper review.

I executed stage2's actual control flow with readback, process creation and flock replaced by logging mocks. Its event sequence is:

```
readback, readback, readback, lock, launch, launch, unlock
```

The wrong result is that engine loading is allowed before mutual exclusion with an already-running timed match. Move the pre-flight under the lock, as the current bl_run_w.sh already does. This reports a new runner reintroducing the same ordering problem; the wrapper's earlier fix is present and correct.

**Ran:** runner_order_review.py. It uses a StringIO object in place of the lock handle and dummy processes, stops after a mocked incomplete block, and touches no actual lock, DLL, production path or other process. Source inspection establishes what the real readback calls launch; I did not perform those loads.

### 3. Prune statistics survive a table-only go and are counted again

**File:** src/search.cpp:221 and 839–843 on search-lab d4370a3; the missing reset is on Engine::go's early-return paths. tools/posbench.cpp adds these stale counts to its totals.  
**Priority:** P2.

Concrete input with the small generated 2–3-piece database and one worker:

```cpp
Engine e;
e.set_hash_mb(1);
Limits l;
l.depth = 5;
l.use_book = false;
e.set_position(Position());             // normal starting position
auto first = e.go(l);

e.new_game();
e.set_egdb_pieces(3);
Position p;
p.set_fen("B:WK1:BK6,K10");
e.set_position(p);
auto second = e.go(l);                   // returned entirely from the MTC tables
auto counts = e.prune_stats();
```

Observed:

```
searched             nodes=2030 stats=308,0,168,0,58,0
table-after-newgame   nodes=0    stats=308,0,168,0,58,0
```

The second call prunes/reduces no moves, so all six counters for that go must be zero. They are reset only in Worker::setup, which the table shortcut does not call. Worker::clear, used by new_game, also leaves qstat intact. A later book-only or no-legal-move return has the same structural gap.

posbench calls new_game for each position and adds e.prune_stats() after each go, so a searched position followed by a covered MTC position adds the first position's statistics twice. Reset these per-call counters before any early return; clearing only in new_game would still leave ordinary successive go calls affected.

**Ran:** stats_review.cpp against the newest search-lab core; output stats-review.log. The fixture was built locally with one thread. No shared database was used.

## 1. Restricted-root and per-engine tag fixes

### Restricted analysis

The fix removes !analysis from the lost-root immediate-draw override, leaving the finite-call restriction. root_moves has already been intersected with searchmoves, so it cannot choose a drawing move outside the requested list. Infinite calls still search the restricted list, where the normal draw logic applies. Without usable MTC, the finite immediate-draw shortcut is still valid.

The updated committed egdb_root_test passes with 121d097's search.cpp. The identical updated test linked with the preceding master's search implementation fails with:

```
FAIL: searchmoves 10-15: the third occurrence is a draw
```

It also checks 10-6 alone still receives the table loss. I reran the separate round-1 history harness with the fixed translation unit:

| Call | Fixed result |
|---|---|
| Original legal history, unrestricted lost root | 10-15, score 0 |
| Same history, searchmoves={10-15} | 10-15, score 0 |
| Same restricted move, infinite=true | score 0, searched |
| Same restricted move, MTC disabled | score 0, searched |
| One prior occurrence, or opposite-side keys | no false third-occurrence draw |
| Lost root with 78 reversible plies, restricted to 5-1 | score 0 |
| Won root's table choice immediately draws | legal move from the fallback search |
| Quiet terminal win at the no-progress boundary | win retained |

The legal repetition input remains: start B:WK21,K25:BK4, play
4-8 25-30 8-11 21-17 11-15 17-13 15-19 13-17 19-15 17-21 15-10 21-17,
then analyse B:WK17,K30:BK10 with those earlier positions as history. I found no remaining defect in this fix.

### EngineGamesA / EngineGamesB

Each counter is indexed by engine A/B, not by the colour it happens to play. Only a launch/relaunch of that engine resets its count. The snapshot is saved before play_game and the counter increments for each scheduled game entered, including a game ending in a crash.

The updated committed runner_test fresh passes on the fixed runner. Against the preceding runner it fails, because the expected A/B tags are absent. The test exercises normal reuse and fresh mode, but does not create an asymmetric restart.

I repeated the original four-game scripted case: A exits in its second game, while B survives. The fixed PDNs report:

| Round | EngineGamesA | EngineGamesB |
|---|---:|---:|
| 1.1 | 0 | 0 |
| 1.2 | 1 | 1 |
| 2.1 | 0 | 2 |
| 2.2 | 1 | 3 |

Independent PID logs confirm that only A was replaced. The leg change does not swap the A/B counts. Thus the original defect is fixed.

**Coverage limit:** the committed test fails before the change because of the tag format, but does not protect the independent-reset logic. A mutation that resets both new counters whenever either process launches still passes runner_test fresh (exit 0). The asymmetric-restart fixture should become a committed regression test. This is a coverage observation; the production implementation passed that case.

## 2. Book selection and repeatability

### Mode 3 has one selectable candidate, including at a score tie

Book::probe performs these operations:

1. Copies the position's entries in their stored order.
2. Removes illegal moves and moves excluded by the book or supplied avoid list.
3. Sorts the remaining entries by score descending, using only score in the comparator.
4. Initializes n=1. It expands n only while margin>0 and the next score is within the margin.
5. Returns entry seed % n.

At **margin 0, n is always 1**. Equal top scores do not create a random choice. Depth and count are not tie breakers in probe. finalize uses depth to retain the deepest duplicate of the *same move*; it does not make the deeper of two different moves preferable during probing.

std::sort does not promise a portable stable order for equal scores. That means the chosen tie need not be a documented cross-build preference. It does not make repeated calls with the same immutable input, executable and library alternate their result.

The retained engine seed advances on eligible probes, including misses, but seed % 1 is always 0. There is no seed-driven difference between consecutive mode-3 games.

### Counts

The requested r11c file is byte-identical to experiments/booklab/wp11c/book_r11c.bin:
SHA-256 14bc3e77d4df26a920d8ea60035fc578bbae316f84b1666e0ebde4aecbfad73f.

| Measurement | r11c requested baseline | r11d on final master |
|---|---:|---:|
| Book entries | 81,061 | 88,195 |
| Distinct position keys | 52,888 | 60,022 |
| Keys with several stored entries | 4,414 | 4,414 |
| Keys with several equal highest-score entries (raw, before avoid filtering) | 257 | 257 |
| Keys with several selectable candidates at margin 0 | **0** | **0** |
| Such margin-0 keys that are pinned/forced | **0** | **0** |
| Such margin-0 keys that are not pinned/forced | **0** | **0** |
| Avoid entries | 5 | 5 |

For r11c I also mapped the pin/decision FENs to keys: 45,916 keys are pinned; another 1,061 have force decisions outside that pin set. None of the 4,414 multi-entry keys or 257 raw top-tie keys is pinned/forced. bookapply removes the other entries of a forced key, leaving one entry. Therefore even if “candidate” meant a raw equal-score alternative, all 257 ties are unforced, and zero are forced/pinned.

### Three concrete tied positions: they choose the same move

There are **no three positions where consecutive identical mode-3 games can get different book moves from the retained seed**. I cannot supply examples of that nonexistent behaviour. Instead I checked three actual ties on best-book continuations from the 156-ballot deck. Both consecutive Engine::new_game/set_position/go calls returned zero-node book answers; probe with seed 0 and UINT64_MAX agreed as well.

1. **ACF #2, ply 7.** Prefix:
   9-13 21-17 6-9 25-21 11-15 23-18.
   FEN B:W17,18,21,22,24,26,27,28,29,30,31,32:B1,2,3,4,5,7,8,9,10,12,13,15.
   Top entries are **12-16, score 11, depth 27**, and **1-6, score 11, depth 28**.
   Both games choose **12-16**. This also demonstrates that the deeper different move does not win the tie.

2. **ACF #2, ply 10.** Continue the prefix above with 12-16 18x11 8x15.
   FEN W:W17,21,22,24,26,27,28,29,30,31,32:B1,2,3,4,5,7,9,10,13,15,16.
   Top entries are **30-25** and **24-19**, both score -11/depth 27.
   Both games choose **30-25**.

3. **ACF #6 (Inferno), ply 6.** Prefix:
   9-13 22-18 10-14 18x9 5x14.
   FEN W:W21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,6,7,8,11,12,13,14.
   Top entries are **26-22** and **24-19**, both score 39/depth 26.
   Both games choose **26-22**.

The traversal found 26 distinct raw top-tie keys on these deterministic book continuations before a book miss or a run of king moves. That is not a count of every tie reachable after all possible opponent deviations.

### Other sources of different play; effect of a fixed node budget

This inventory concerns master search.cpp, cb_api.cpp and tt.cpp under identical executable, evaluator, database and option settings.

| Source | What can differ | Removed by a fixed node budget? |
|---|---|---|
| Wall-time hard/soft stopping | OS scheduling, CPU/cache/page-fault speed and callback/output overhead change the amount of search completed; stable-move and score-drop time policy can stop at another iteration | **Yes**, if all time limits are removed and no external stop intervenes |
| Time offered on later moves | A previous search's runtime changes the host's bank and later maxtime/remaining arguments | **Yes** for an exclusively node-limited experiment; setting nodes alongside a live time limit does not remove the clock |
| External playnow / stop_poll / stop() | Host cancellation can cut a different partial iteration or arrive before the first score | **No**, unless those external stops are held constant/disabled |
| Multiple search workers | Shared hash-table contents and cutoffs depend on worker scheduling; helpers search different depths | **No**; use one worker for deterministic node comparisons |
| Multiworker node-limit approximation | The main worker checks its own count times the thread count, rather than a synchronized global budget; helper work and stop timing differ | **No** |
| TT retained between moves | Move ordering, cutoff scores, static evaluations, replacement depth and generation depend on earlier searches in the same game | **Not by itself**; fixed nodes makes a one-thread identical preceding search repeatable but does not clear its results |
| Quiet history, continuation history and killers retained between moves | Earlier searches affect ordering and reductions | Same qualification as TT |
| Game history, reversible count and inferred after-position | Repetition and no-progress values depend on the exact game history supplied | **No**; reproduce the complete history, not just the root FEN |
| Book seed retained across games | It can choose another entry when positive margin admits several moves | **No** in modes 1/2; **inactive at margin 0** |
| Different configuration retained in a process | Hash size, evaluator, threads, book/avoid data, parameters and loaded database affect play | **No**; these must actually be identical |
| Database/TT/evaluator cache residency | Changes search speed in timed play; database result itself is unchanged | Speed effect removed by an exclusive node budget |
| Fixed node polling interval and partial result | Main-thread checks occur every 1,024 nodes, so the count is not an exact one-node boundary | Deterministic with one thread and identical state; it is an overshoot, not fresh randomness |

On CB_RESET_MOVES our engine clears TT and its generation, quiet/continuation history and killers. A following set_position replaces root/history keys. Supplied gamehist is consumed; a reset prevents inferred history from the preceding game from being extended. Worker::setup resets per-search roots, counters, depths, partial state and the NNUE accumulator. No time bank exists in this CB adapter; clocks come from each call's arguments.

Loaded book/evaluator/database, options and the book seed survive. I found no additional active cross-game search state in these three files that changes mode-3, one-worker play after a proper reset and identical history. Hash generation/replacement and equal-order sorting are deterministic with one worker and identical inputs. Timing can first change a search, after which retained *within-game* TT/history and later clocks amplify the difference. That does not imply those search tables survive a new game.

This repeatability statement does not override the ledger's accepted plain-int host stop flag or history-dependent hash approximations.

## 3. Book Lab win-mining tools, stage gates and process-state scripts

### Six Round-21 fixes

I read the 0ae8af2 changes and rechecked the affected code at newest Book Lab 1a1b654. The original causes were reproducible from the ledger, except that the exact “20 affected selected points” campaign inventory was not fully regenerated. Tests below are small fixtures, not reruns of the timed campaign.

| Ledger item | Recheck | Result |
|---|---|---|
| 1. Per-line cap ignores another validation repeat | Four eligible points, nominated on different lines, all also reaching ACF #1 B in another Cake repeat; scores mocked equally valid | Only p1,p2,p3 selected; p4 blocked by the three-per-line cap |
| 2. Known point bypasses eligibility | Actual W30 FEN tested as protected; already playing 8-12; fewer than two non-avoided moves; avoided 8-12; missing pinned score; missing alternative score | All produce no candidate and a specific reason |
| 2. Known point failing only the score margin | Pinned score 100, 8-12 score 0 | Retained as the explicitly allowed separate own arm, rather than a missing-score “None” candidate |
| 3. A searched equal move counted as consultation | Eligible row and expected book move, but comment depth 21/nodes 812345 instead of depth 0/0/nodes 0 | consulted=0; the corresponding book comment gives consulted=1 |
| 3. Walked book actually plays another move | Played move equals candidate alternative but pinwalk reports another book move | consulted=0 |
| 4. “lost on time” not recognized | Cake 1.89f lost on time (0.9s used, 0.5s left) | on_board=False; cannot nominate a discovery win |
| 5. Failed replay reported as clean | Won game, lost game and reference pool emit pinwalk ERR | Both reporters explicitly print REPLAY FAILED and exit 3 |
| 5. wins_vs_book drops FEN | A FEN-started game passed to a fixture that refuses an empty/wrong FEN field | Correct FEN arrives; the reporter reaches the expected ERR handling, exit 3 |
| 6. Contexts frozen before validation complete | Read workflow and compare stored old/refreshed point tables | Wait moved ahead of point generation, but completion predicate still has finding 1 |

The selector tests run the real selection script with a small scripted scorer, --threads 1, a private one-file dummy database manifest, and no engine search. Reporter tests run the real Python reporters with a scripted pinwalk ERR response; they validate propagation and FEN forwarding, not the C++ replay algorithm.

Stored c1_points_r11c.tsv and refreshed c1r/points.tsv.gz each contain 5,298 points. Comparing context sets shows **4,865 points gained contexts**. For example P267 gained the ACF #169 B GuiNN context cited in context_refresh.txt, as well as additional Cake contexts. I did not regenerate the entire frozen campaign and its original exact 20-selected-point claim; the available considered table is labelled partial. The recreated cross-repeat cap case and known-point cases test the described causes directly.

The inherited avoid-in-king-runs issue is assigned in the ledger and not reported anew; newest 02de695 modifies those expressions separately.

### Stage 2 reference cases

Ran the committed tests_wp12_gates.py: **20 tests passed**. The requested gate cases give:

| Candidate | Exposed wins | Control wins | Blocks | Verdict |
|---|---:|---:|---:|---|
| 4 W / 8 D / 0 L | 4 | 1 | 3 | keep |
| 4 W / 8 D / 0 L | 3 | 1 | 3 | reject |
| 4 W / 7 D / 1 L | 4 | 1 | 3 | reject |
| 4 W / 8 D / 0 L | 4 | 2 | 3 | reject |
| 4 W / 8 D / 0 L | 4 | 1 | 2 | reject |

Incomplete totals or off-board terminations yield “not counted”. Only consulted candidate wins contribute to the four-win minimum and the three-win advantage; all candidate losses reject, including losses without consultation. These calculations match master's PROTOCOL.md.

### Stage 3 and runner

The plan schedules KingsRow and GuiNN for the nominating context, then all three opponents for every other recorded context, 12 games per pair. The tests check plan construction, exposed-loss rejection, an unexposed loss being reported separately, incomplete pairs not counted, and fewer than six consultations being labelled untested. I found no error in those tested gate calculations. A “pass” can contain explicit untested notes and is not proof of safety.

The stage-2 runner writes a repeated fixed-colour deck and starts candidate/control matches together per block, four games and four workers in each by default. Thus each worker plays one real game in that block even without --fresh-engines; there are three separate process-start blocks. Both arms use the same Cake options, 0.5-second banking control, book mode 3 and offered concurrency. It records DLL/book/runner/host hashes and rejects a changed stored manifest.

I inspected command construction, deck writing, result completion, analysis and gate application. I did **not** run production stage2/stage3, because their paths and side effects belong to the protected match jobs. Finding 2 is demonstrated through mocked control flow. The stage-3 launcher delegates to the normal wrapper; the gate checks expected game counts and on-board results.

### Process-state arms

Read newest e2_chain.sh and its bl_run_w.sh path. For the ACF 163 experiment both arms use the same 24 fixed-White deck lines, Regicide DLL bl-ddd25fc-r11d (n8q), Cake 1.89f, match-ddd2, ddd2 cbhost, hash 128, Regicide one thread, explicit book 3 with read-back, opponent database options, 0.5 s/move banking, PDN comments and concurrency 8. The substantive command difference is --fresh-engines; tags/output directories differ as identifiers. The full-deck arms similarly use identical halves and repeat settings, in kept/fresh order.

Fresh mode necessarily adds engine reinitializations and warm-up searches. Those are part of the intervention, not an unnoticed second engine setting. The arms run at different times; the scripts cannot make realized CPU/load/cache conditions identical. The order is fixed, not randomized or counterbalanced.

The final fetch also brought Book Lab 1a1b654 and e2c_resume.sh. The affected review sources are unchanged. The resume script retains the original DLL, runner, host, clocks, deck halves and concurrency, with only --fresh-engines distinguishing its two arms. Its incident note archives a completed half-run whose wrapper lost its script and failed to write exit_status; the script replays that half-run. I read these changes and did not run the resume or health scripts.

The newly added mo_chain.sh also uses identical settings for each KingsRow fixed-White fresh/kept pair: same DLL bl-1c6e997, runner/host, opening, 24 games, colours, clock and concurrency. The “natural pairs” arm deliberately adds Black games and --pairs-together, so it is **not** a fresh-versus-kept arm with all other conditions identical. mo1_chain.sh deliberately uses concurrency 1 and NOLOCK; it is a separate match-order experiment, not a matched fresh control. I did not treat these declared interventions as defects.

## 4. prune_forcing

The switch's implemented contract is narrower than “all pruning”: mode 1 exempts forcing quiet moves from **late-move pruning and per-move futility**; mode 2 also exempts late-move reductions; -1 counts forcing candidates while behaving as mode 0. Reverse futility and other whole-node decisions are unchanged. “Quiet” here excludes captures and promotions; those already bypass these quiet-move rules.

### Baseline reproduction

Built master core and search-lab with the same Clang, Release settings, native optimization, LTO off, one worker and no external database.

- Master bench: **7,833,556 nodes**.
- Search-lab default prune_forcing=0 bench: **7,833,556 nodes**.
- All **17 per-position rows**, including best move, score and nodes, match exactly.
- On the 64-position Q set at depth 8, mode 0 and mode -1 both use **458,708 nodes**, with identical per-position scores/moves/node counts. Only elapsed times and the diagnostic forcing counts differ.

This verifies the default path on the benchmark and the 64-position comparison; it does not claim exhaustive equivalence on every board or with other search-lab switches enabled.

### Which moves are exempted

Concrete position: **B:W18:BK1,10**.

| Black move | Does White have a mandatory capture afterward? | Exemption when this move reaches an eligible LMP/futility decision |
|---|---|---|
| 10-15 | yes (18x11) | kept by mode 1 |
| 1-5 | no | still pruned |
| 1-6 | no | still pruned |
| 10-14 | yes (18x9) | forcing, but first-ordered in this fixture and therefore never an eligible late-move prune |

To isolate each rule, I invoked the real Worker::search at a non-PV node, depth 1, window [20000,20001], HCE, database off and qext=0. For LMP I set lmp_base=-5 to make every later quiet eligible; for futility I disabled LMP and set fut_margin=0. These artificial parameter/window choices test the predicate, not playing strength.

| Rule / mode | Nodes | Rule candidates | Forcing candidates kept/countable |
|---|---:|---:|---:|
| LMP / 0 | 3 | 3 | 0 (not inspected) |
| LMP / -1 | 3 | 3 | 1 (count only) |
| LMP / 1 | 5 | 3 | 1 (searched) |
| Futility / 0 | 3 | 3 | 0 (not inspected) |
| Futility / -1 | 3 | 3 | 1 (count only) |
| Futility / 1 | 5 | 3 | 1 (searched) |

The code tests the child position and the child's side to move, so it recognizes the opponent's capture after our move. It does not mistake an existing threat before the move for a mandatory capture afterward. Mode 1 leaves LMR active, as documented. In the sampled 64 positions mode -1 counted LMP 65,218 / forcing 26,724; futility 138,467 / forcing 49,193; LMR 40,265 / forcing 15,149.

The primary counters intentionally count candidates *before exemption*, not just moves finally pruned/reduced; the public header documents that meaning. Each eligible rule increments once, and the forcing subcounter increments only when its child has a capture. I found that counting logic correct in the direct fixtures. Finding 3 concerns lifecycle/reset, not the capture predicate or these increments.

## Verification limits and delivery

Focused Linux/WSL builds, tests, scripted protocol games, small database probes and source reads were performed. No full ctest suite, Windows DLL execution, strength match or shared 2–8-piece database load was performed. No experiment results were produced by running the protected launchers.

Primary local evidence is in reports/round2/: book-review.log, book-counts.log, root-completeness.log, root-before.log, tags-before.log, tags-mutant.log, pid-crash/games.pdn and its PID log, gates-tests.log, mining-review.log, replay-failure-review.log, context-refresh-review.log, context-completion-review.log, runner-order-review.log, bench-compare.log, master-bench.log, master-final-bench.log, search-bench.log, pf0.log, pf-minus1.log, forcing-review.log and stats-review.log. The concrete inputs and outputs needed to assess the findings are included above so the report is usable without access to this machine.

Only this text report is added for round 2 to t3/review-reports. It is below 1 MB. Source extracts, binaries, database files, fixtures and build logs are not committed.

