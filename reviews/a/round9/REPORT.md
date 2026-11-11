# Review A — Round 9

Date: 2026-11-11. Publication: reviews/a/round9/ on t3/review-reports.

The bounded differential checks found no mismatch in the core or CheckerBoard move output. The claim-boundary branch fixes the 78-ply mate claim and the stopped table-draw fallback, and its bench remains 7,833,556 nodes. There is one documentation finding: the setup page omits the GUI/runner difference when a terminal winning move carries a draw claim.

There are no mo2 opponent-draw-claim endings meeting the requested last-search score >= +100 filter. This is also true if zero-node book/table comments are included as reported scores. The paired end-score list for all 24 changed results is below.

## Revisions, ledger and execution

| Source | Checked revision |
|---|---|
| origin/master at start | fdd154e17db15edd92dea51be3d332eccf8f2078 |
| origin/master after ledger update | fa1fc5718b8642aa0a9796a06d49837bb6475ad1; PR #80 changes only the ledger |
| origin/t3/claim-boundary | e027cf4975a15261824a34cf72338d264b8ce852 |
| Record branch | 7ca6c27198ea0cc89f315744448fdc005ea81423; original mo2/cl files unchanged from round 8 |
| CheckerBoard reference | 32f194f840f208f0aff86a07026f788c01c819ce, version 1.78 |

The ledger was read first, again at PR #80, and after the final fetch. The two code items assigned to the integrator are reported as verified, not as new defects. The only new finding is a missing qualification in the newly added setup page.

Prior coverage not repeated in the same form: the NNUE forward-arithmetic/scalar/SIMD comparisons, colour-flip symmetry, loader/header/overflow tests and threaded-interface checks in B7/B12/B13; CheckerBoard ABI/layout/calling-convention checks from round 18; TT move-list permutation/restricted-root identity cases already fixed and checked with PRs #31/#33. The new n8q test compares raw accumulator lanes, including sibling reuse and bucket crossings, rather than repeating only scalar-output/symmetry checks. The interface test exercises all legal move outputs in its corpus rather than another single-move layout smoke test.

All builds, tests, engine calls and analysis drivers ran foreground with nice -n 19, at most two build jobs and one active search worker. Each command finished within ten minutes. Everything, including TMPDIR and private database fixtures, was under /home/admingod/regicide-review-a. No real opponent engine, protected live directory, shared database, lock or other agent's process was used. Scripted claim fixtures contain no search. The record work parses existing comments only and runs no engine evaluation.

Master core source/weights are byte-identical to the private release core built/tested in round 8, so that static library is reused for master drivers; each new driver is freshly compiled. Current master additionally changes only the ledger. claim-boundary has a fresh Clang/CMake release build, LTO off, -j2. The test-only drivers include the actual source when they need private file-local functions; production algorithms are not rewritten or patched.

## Finding 1 — the setup page omits a terminal draw-claim difference

File: [docs/CHECKERBOARD_MATCH.md:35–48](https://github.com/AdminGodZ/Regicide/blob/e027cf4975a15261824a34cf72338d264b8ce852/docs/CHECKERBOARD_MATCH.md#L35).

The claim-handling correspondence and remaining-differences list omit the case where the played move leaves the opponent without a move but the DLL returns CB_DRAW. The pinned GUI and our runner give different results for the same input:

- Board B:W10:B6, Black plays the only legal move 6x15, and getmove returns CB_DRAW (0).
- GUI, engine match, early adjudication on, limit 200: ends in attempt 1, PDN 1/2-1/2.
- Runner, --cb-claims --cb-move-limit 200: ignores that contradictory draw claim in favour of the board win, PDN 1-0. Both scripted pair legs give the same board result.

I compiled and ran the actual pinned SearchThreadFunc and update_match_stats for that terminal board/return. SearchThreadFunc checks rule draws then accepts CB_DRAW at [CheckerBoard.c :3578–3580](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L3578); it has not yet made another attempt to find the opponent without a move. Our tools/match/main.cpp instead keeps the terminal board result. A separate actual runner/host call with our logging DLL reproduces 1-0.

This is a documentation qualification, not a request to remove the runner's intended terminal-board priority or a new failure of the two code fixes. Add the precedence difference to the page's remaining differences; no GUI setting makes the conflicting-claim cases identical. The normal terminal CB_WIN case now agrees, as the new fix intends. No mo2 ending in the checked records uses this contradictory terminal draw-claim case.

Evidence: [terminal-draw-reference.log](evidence/terminal-draw-reference.log), [terminal-draw-runner.csv](tables/terminal-draw-runner.csv), and [terminal_draw_reference.py](fixtures/terminal_draw_reference.py). The reference adapter stubs UI services and getmove but executes the actual result-handling functions.

## Task 1 — seeded differential checks on master / built-in n8q

### a, b, d: one combined board/move/accumulator/interface driver

[core_diff.cpp](fixtures/core_diff.cpp), seed 0xA9C0DE64, uses 160 bounded random legal playouts from the ordinary start plus 4,000 seeded valid-material layouts, 2–24 pieces, both colours, men/kings, at most twelve pieces per side, no uncrowned man on its crowning row. Layouts also set reversible counts in 0–78 to exercise counter updates; they are structural cases, not claimed reachable game histories. Playouts stop on terminal boards, rule draws or the 200-ply bound, and reset the accumulator stack after 96 plies to stay within the search's stack size.

At each input position it compares the engine's complete legal move set, by from/to/captured set, against tools/common/checkers.cpp, checks compulsory capture, and tests every legal child, not only the randomly chosen continuation.

| Check/corpus measure | Count |
|---|---:|
| Input-position visits | 16,117 |
| Side to move Black / White | 8,069 / 8,048 |
| Legal child makes checked | 77,680 |
| Raw accumulator comparisons, including parent rechecks | 109,914 |
| Compared active int16 accumulator lanes | 225,103,872 |
| Board-array round trips | 16,117 |
| Complete write_move/path checks | 77,680 |
| Sibling stack-slot writes | 77,680 |
| Quiet moves | 69,275 |
| Single captures | 7,006 |
| Multiple captures | 1,399 |
| Promotions | 2,928 |
| Promotions on a capture | 583 |
| King moves | 29,777 |
| Captured kings | 3,386 |
| Captures returning to their origin | 4 |
| Mismatches | 0 |

Categories overlap; input visits are not a distinct-position count. The built-in net reports v1, FT 1024, hidden 32/32, eight output buckets.

Evaluation: for every child and the restored parent, AccumulatorStack's raw active lanes are compared with Network::refresh from scratch for both colour perspectives. For the v1 network the driver explicitly requests the other point of view too, so an equal saturated output cannot hide a wrong unused FT vector. It also compares incremental/full-refresh output scores at the child's actual side and piece-count bucket. Parent accumulators are rechecked after siblings overwrite the next stack slot.

Position: child piece sets, king set, side, key and key_after are compared with a fresh Position constructed from the independent referee's resulting FEN. The reversible count is independently expected to reset for any capture/man move and increment only for a quiet king move. The parent is unchanged after each child-copy operation. There is no Position::unmake_move in this engine: Worker::search copies Position before make, and overwrites/reuses accumulator entries for siblings. Thus copy/backtracking reuse is checked; an unmake operation is not claimed tested.

Interface: the driver includes the actual src/cb_api.cpp helpers in its test translation unit. It converts the independent Board8x8 through read_board, reconstructs the array through the actual coordinate/piece helpers, and compares with the source board. For every legal move, write_move's board, endpoints, old/new piece code, jump count, capture coordinates/codes and intermediate path are checked. CBmove starts filled with nonzero bytes, so unused output fields must also be cleared.

The path check is an independent scalar walk over 8x8 coordinates, removing each actual victim and testing direction, occupancy and stopping on promotion. This accepts any legal route with the same captured set instead of demanding that two generators choose the same representative of equivalent circular capture paths. path[0] is unused and intermediate landings begin at path[1], as specified by the pinned cb_interface.h; captured-piece array order is not assumed to be jump order.

All eight buckets and all seven adjacent downward boundaries occur. The complete observed bucket transition counts, including two-bucket drops, are:

| Parent bucket | Position visits | Checked child transitions |
|---|---:|---|
| 0 | 1,341 | 0→0: 4,560 |
| 1 | 3,529 | 1→0: 91; 1→1: 16,919 |
| 2 | 2,383 | 2→1: 252; 2→2: 11,471 |
| 3 | 1,975 | 3→2: 340; 3→3: 9,825 |
| 4 | 1,798 | 4→2: 1; 4→3: 443; 4→4: 8,882 |
| 5 | 1,684 | 5→4: 587; 5→5: 8,128 |
| 6 | 1,490 | 6→4: 4; 6→5: 682; 6→6: 6,132 |
| 7 | 1,917 | 7→6: 686; 7→7: 8,677 |

This states the sampled transitions, not a claim to enumerate every mathematically possible multi-bucket capture. Every change encountered in the seeded corpus is checked. [core-diff.log](evidence/core-diff.log) contains the raw counts. No evaluation/position/move/interface counterexample was found.

### c: TT score distances, bounds/replacement and illegal move references

[tt_diff.cpp](fixtures/tt_diff.cpp), seed 0xA9D157, compiles the actual score_to_tt/score_from_tt helpers from master search.cpp and uses the real TranspositionTable.

| Check | Cases/result |
|---|---|
| Positive/negative mate and table-score store at p / probe at q | 8,892; normalized distance correct |
| Packed score, eval, depth, bound and move round trips | 20,000; exact field equality |
| Same-key depth/exact-entry and generation preference | 960; expected two-ply leeway, exact replacement, stale replacement and old-move retention |
| Forced four-entry bucket replacement | 2; shallow victim and age preference observed |
| Actual search cutoff windows | 4; exact/lower/upper narrow-window cutoffs, and no exact-TT cutoff in a PV window |
| Full searches with illegal TT move references injected at root and children | 96, one worker, depth 3 / at most 2,000 nodes |
| Injected references individually rejected by find_move | 613 |
| Returned moves / PV plies | All returned moves legal; 376 PV plies legally replayed |
| Mismatches | 0 |

The distance oracle retains the underlying mate/table distance and changes only root-relative ply: positive score base-distance-q, negative score its opposite. Ordinary non-decisive fields are tested in the independent round trips. Depth cases cover negative search depths through 127 and all real bound types.

The poisoned move's origin is empty in the probing position. It cannot denote a legal move there. The driver verifies find_move returns -1, then runs the actual search with the poisoned table and legally replays its full returned PV. The table is accessed through Engine::hash on an otherwise non-const test engine; no production search code is replaced. These tests do not repeat the accepted history-dependent-TT/no-progress approximations or the earlier concurrent-table checks.

Raw output: [tt-diff.log](evidence/tt-diff.log).

## Task 2 — claim-boundary branch and setup page

### Claim and stopped-call fixtures

| Fixture | Before, master | e027cf4 |
|---|---|---|
| No-table B:WK10:BK1,K6, 6x15, 78 legal reversible plies | Mate 29999, CB_UNKNOWN 3 | Same mate, CB_WIN 1 |
| Same history with first reversible ply removed, count 77 | CB_WIN 1 | CB_WIN 1 |
| Fresh same root, count 0 | CB_WIN 1 | CB_WIN 1 |
| Stopped-before-call B:WK30:BK25,K26, private 3-piece DB | 25-21, child is opponent table win | 26-22, child is table draw |
| Stopped table-draw claim return | CB_UNKNOWN 3 | CB_UNKNOWN 3, correct: no rule draw has been completed merely by the table's draw label |

Both getmove fixtures are direct actual-DLL calls. No-table histories are the same legal/distinct 78-king-ply fixture from round 8; the mate is a capture of White's last piece. The new public cb_api_test fails on the preceding master DLL at the 78-ply claim. The independent filter driver also fails on master: expected three holding root moves, got all six, fallback 25-21.

All earlier claim controls pass through getmove on the branch: fresh short Black/White wins; fresh MTC win; late 77-ply table win not claimed; already-third-occurrence root not claimed won; our move completing third occurrence returns CB_DRAW, second occurrence does not; quiet move at root count 78 draws at 79, while root count 77 does not; the searched-table half-window 4/37/38 tests; and the opponent's drawing-reply history suppresses CB_WIN. The stopped-call fallback is tested here because the lead explicitly requested review of its fix, not re-reported as an open issue.

Executed cb_api_test and egdb_root_test both pass. Bench is exactly 7,833,556 nodes, with all 17 move/score/node rows equal to the preceding master baseline. [api-cases-after.tsv](tables/api-cases-after.tsv), [terminal-clock-after.tsv](tables/terminal-clock-after.tsv), [stopped-before.log](evidence/stopped-before.log), [stopped-after.log](evidence/stopped-after.log), [claim-api-test.log](evidence/claim-api-test.log).

### Table-drawn root filter

[filter_diff.cpp](fixtures/filter_diff.cpp), seed 0xA9F17E, uses the private complete two/three-piece WLD/MTC fixture. It compares the actual root list retained inside Engine::go with children generated by the independent tools/common referee and freshly reconstructed/probed.

The driver uses test-only read access to the real worker root list; it does not substitute a second filter implementation into the engine. Every tested call is stopped before it can score a root move, so the fallback exercises the list that will actually be used.

- All 6,976 non-overlapping two-piece boards with one piece per side, all valid man/king choices and both sides to move, excluding uncrowned men on their crowning row.
- All 59,520 two-kings-versus-one-king layouts, both material colours and both sides to move.
- 4,000 seeded three-piece mixed layouts and the named stopped-call case.

Total: 70,497 input cases, 8,027 table-drawn root cases. Their 15,358 legal moves partition into 1,705 removed opponent-Win children and 13,653 retained Draw children. No non-Win child was removed, no Win child retained, and no list was empty. The special stopped root retains exactly three of six moves. Infinite analysis and a restricted analysis list on that root retain their original requested lists.

No child Loss/Unknown is observed in this complete, coherent small-table drawn-root corpus. Loss would imply a winning root move; the source explicitly retains Unknown via != Win, but that failure-of-probe path is read rather than claimed executed. The defensive all-filtered fallback is likewise read; no coherent drawn root in this corpus required it. The result is bounded to these small-table cases, not an exhaustive 2–8-piece certificate.

Raw output: [filter-diff.log](evidence/filter-diff.log); before-code failure: [filter-before.log](evidence/filter-before.log).

### Setup-page trace review

Except finding 1, the new page agrees with the round-8 trace:

- Identical DLL/embedded weights; fresh defaults book 3, margin 0, hash 128, one worker; no GUI setter at load/match start.
- Module-relative egdb, manual path before a different successful load, get dbpieces 8 as the required WLD coverage for the full set, no engine-option persistence across sessions.
- GUI searchthreads control disabled for our threads spelling; book/hash controls and manual threads command are supported.
- 0.5 s, non-incremental, handicap off, exact-time checkbox irrelevant in match mode, early adjudication on.
- Userbook off, cross-board plus mail-play/barred off, ordered 156 deck, primary Black first, reversed-colour pairs, fresh match.
- Runner -c 1, pairs together, processes kept; current --openings acf156 is accepted by its opening loader.
- Warm-up, repetition boundary, automatic timeout policy, four-decimal maxtime serialization, process layout and working-directory differences are supported by the inspected source.

The page's “200 searches per game” should be read as an adjudication threshold, not a guarantee that getmove is called at most 200 times: the GUI may make the N+1 attempt and record unknown. That behaviour was already established in rounds 5–8; it is a wording precision note, not a new code finding.

## Task 3 — end scores in the census and mo2 records

This is fresh parsing of the nine committed PDNs (six census halves and three mo2 runs), 1,872 games, with decompressed hashes checked against round 8. No engine evaluation is run. The 24 comparison keys and their game matching are the unchanged round-8 results, not inferred from file order.

“Last search” means our latest own comment with nodes > 0 within the stated endpoint. “Last reported” also includes zero-node book/table moves. A reported score describes the position we searched before our own move; it is not a new evaluation after the opponent's final claim move. Values are in centi-men from Regicide's perspective.

The census has two relevant endpoints: the first accepted claim used by round 7's counterfactual, and the end of the actually played-out census game. Both are reported. mo2 ends at its actual claim/rule result. In the table, a cell is endpoint ply; last-search score@its ply. Where the final own reported score differs or no search exists, the cell also supplies that last reported value with its ply and a zero-node marker (superscript 0).

| Opponent / ACF / our colour | Census half: game | Census first-claim endpoint | Census played-out endpoint | mo2 game / result | mo2 endpoint |
|---|---|---|---|---|---|
| cake / 143 / B | h2: 101 | 89; 19991@89 | 169; 19999@97; last 20000@169⁰ | 257 / D | 102; 0@101 |
| guinn / 5 / W | h1: 8 | 50; 19970@50 | 200; 19998@134; last 20000@200⁰ | 8 / D | 55; 0@54 |
| guinn / 22 / B | h1: 39 | 55; 19976@55 | 145; 19996@85; last 20000@145⁰ | 39 / D | 36; 0@35 |
| guinn / 23 / W | h1: 42 | 43; no search; last 0@42⁰ | 74; 0@74 | 42 / W | 59; no search; last 0@58⁰ |
| guinn / 25 / W | h1: 46 | 57; 52@24; last 0@56⁰ | 230; 19995@168; last 20000@230⁰ | 46 / D | 43; no search; last 0@42⁰ |
| guinn / 27 / W | h1: 48 | 128; 19974@128 | 234; 19998@184; last 20000@234⁰ | 48 / D | 45; 0@44 |
| guinn / 31 / B | h1: 55 | 119; 19982@119 | 237; 19998@155; last 20000@237⁰ | 55 / D | 48; 0@47 |
| guinn / 40 / W | h1: 72 | 43; no search; last 0@42⁰ | 78; 0@78 | 72 / W | 82; 19976@82 |
| guinn / 64 / W | h1: 112 | 31; 668@30 | 202; 19998@136; last 20000@202⁰ | 112 / D | 45; 0@40; last 0@44⁰ |
| guinn / 65 / W | h1: 114 | 34; 19976@34 | 240; 19998@170; last 20000@240⁰ | 114 / D | 37; no search; last 0@36⁰ |
| guinn / 71 / W | h1: 126 | 45; no search; last 0@44⁰ | 88; 0@88 | 126 / W | 37; no search; last 0@36⁰ |
| guinn / 74 / W | h1: 132 | 58; 19971@58 | 200; 19999@138; last 20000@200⁰ | 132 / D | 37; 0@36 |
| guinn / 78 / W | h1: 138 | 43; 0@42 | 86; 0@86 | 138 / W | 35; 686@34 |
| guinn / 86 / W | h1: 154 | 109; 0@108 | 236; 0@236 | 154 / W | 78; 19978@78 |
| guinn / 90 / W | h2: 4 | 51; 467@46; last 0@50⁰ | 178; 19998@102; last 20000@178⁰ | 160 / D | 47; 0@46 |
| guinn / 92 / B | h2: 7 | 36; no search; last 0@35⁰ | 124; 0@123 | 163 / W | 81; 19970@81 |
| guinn / 96 / W | h2: 14 | 57; 0@56 | 93; 0@92 | 170 / W | 62; 19972@62 |
| guinn / 131 / W | h2: 80 | 38; 19963@38 | 204; 19999@144; last 20000@204⁰ | 236 / D | 73; 0@72 |
| guinn / 133 / W | h2: 84 | 90; 19976@90 | 194; 19998@138; last 20000@194⁰ | 240 / D | 55; 0@54 |
| guinn / 149 / W | h2: 112 | 36; 19970@36 | 154; 19998@82; last 20000@154⁰ | 268 / D | 37; no search; last 0@36⁰ |
| guinn / 167 / B | h2: 145 | 36; 0@35 | 112; 0@111 | 301 / W | 39; 19978@39 |
| guinn / 174 / W | h2: 156 | 109; 2@108 | 182; 0@182 | 312 / W | 51; 439@50 |
| kingsrow / 16 / W | h1: 28 | 59; 494@56; last 0@58⁰ | 244; 19994@168; last 20000@244⁰ | 28 / D | 199; 0@198 |
| kingsrow / 134 / W | h2: 86 | 72; 19976@72 | 184; 19998@120; last 20000@184⁰ | 242 / D | 67; 0@66 |

The first-claim and played-out census endpoints differ because the census ignored claims. The mo2 W/D changes in the table are the already verified 24 opening/colour differences, all preceded by a different opponent move; they are not identical lines stopped at a different point. [changed-result-end-scores.tsv](tables/changed-result-end-scores.tsv) contains full run names, game numbers, every endpoint ply, last positive-node search score/ply/nodes and last own reported score/ply/nodes. Where no own search exists, the last own reported zero-node score is explicitly supplied rather than replaced with a presumed search.

### All mo2 opponent draw claims with last own searched score >= +100

The requested list is empty.

| Opponent | Draw-claim endings | With a prior own searched score | No prior own search | Last-search score range | Highest last reported score, including zero-node moves | Qualifying >= +100 |
|---|---:|---:|---:|---|---:|---:|
| Cake | 312 | 191 | 121 | -1 to +13 | 0 | 0 |
| KingsRow | 306 | 292 | 14 | 0 to 0 | 0 | 0 |
| GuiNN | 292 | 171 | 121 | 0 to +6 | +5 | 0 |
| Total | 910 | 654 | 256 | -1 to +13 | +5 | 0 |

There are six additional mo2 board-repetition draws, outside the requested opponent-claim filter. Every eligible claim is by the opponent and is on the final recorded ply; matching FirstClaim and CSV tags were checked. The 256 games without a prior searched score are not silently treated as a search returning zero. Even when the last reported zero-node score is used instead, no ending reaches +100.

The full audit of all 910 eligible games is [opponent-draw-claim-end-scores.tsv](tables/opponent-draw-claim-end-scores.tsv); the empty requested subset is [draw-claims-plus100.tsv](tables/draw-claims-plus100.tsv), with a header only. [end-score-summary.tsv](tables/end-score-summary.tsv) and [end-scores.log](evidence/end-scores.log) give counts/ranges.

The records therefore supply no mo2 case meeting this high-last-search-score criterion. They do not prove every final claimed position drawn: the engine has not searched again after the opponent's final move, and the games stop at the claim. No later win is inferred or ruled out by evaluating a position here.

## Reproduction and evidence

Source revisions, final ledger fetch and code/record hash checks: [source-heads.txt](evidence/source-heads.txt).

- [core_diff.cpp](fixtures/core_diff.cpp) includes master-src/src/cb_api.cpp and links the master core plus tools/common/checkers.cpp; seed and bounds are fixed in the driver.
- [tt_diff.cpp](fixtures/tt_diff.cpp) includes the actual master search.cpp to reach its file-local distance helpers and Worker search windows; it links the remaining core archive members.
- [filter_diff.cpp](fixtures/filter_diff.cpp) includes the actual claim-src/src/search.cpp; compile with BEFORE to use master instead. The only test access shim reads the private worker root list, without changing the production class or filter.
- [api_cases.py](fixtures/api_cases.py), [api_terminal_clock.py](fixtures/api_terminal_clock.py), [api_stopped.py](fixtures/api_stopped.py), and [census_referee.cpp](fixtures/census_referee.cpp) reproduce the direct-DLL and independent board fixtures. The 78-ply terminal and prior histories are included in fixtures/histories; the small database is the private test_egdb.hpp fixture.
- [reference_adapter.py](fixtures/reference_adapter.py) extracts the pinned public source with UI/getmove stubs; [terminal_draw_reference.py](fixtures/terminal_draw_reference.py) generates the terminal draw-return case. Set REVIEW_OUT=reports/round9 and CHECKERBOARD_SRC to the pinned checkout before generating the reference adapter. No real opponent engine is involved.
- [end_scores.py](fixtures/end_scores.py) parses the nine PDNs and the included round-8 pairing/hash manifests. It does not run any engine. The fixture layout is the private reports/round9 directory; report data and manifests are portable text.

All committed files are text, below 1 MB. No engine source branch, production record or ledger is modified.
