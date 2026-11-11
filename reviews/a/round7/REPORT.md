# Review A — Round 7

Date: 2026-11-11. Report destination: reviews/a/round7/ on t3/review-reports.

One engine's draw return is enough to end a CheckerBoard engine-match game, including when early adjudication is off. It does not require the other engine's agreement, a material threshold or a minimum move number.

There are two findings on the claim-soundness branch: the new win guard misses a repetition available to the opponent on its next reply; the runner discards a terminal win claim on the last allowed attempt. The four original round-6 fixtures now pass, with the two completeness failures detailed below. The bench is unchanged at 7,833,556 nodes. I found no false CB_DRAW in the draw-guard checks.

The strict-record near-miss list has 13 drawn games, all with Regicide Black: nine against Cake and four against KingsRow. The claims census arrived at e4d7908. All 936 games and 101,794 moves were independently replayed. No first claim disagrees with the played-out result; applying --cb-claims to those recorded games preserves every result.

## Sources and execution

The ledger on origin/master was read before review and again after fetching. Accepted matters and findings already assigned to an owner are not reported again. The two findings below are failures of the proposed fixes, with new concrete inputs.

| Source | Revision |
|---|---|
| origin/master | 8d76ba665956b1c5fc8d0a6b86bb1eb40becfd76; PR #78 adds the A4–A6 ledger, engine/runner code unchanged from the supplied 8de39d0 |
| origin/t3/claim-soundness | e252fd0db300776815dc5d32b990a8a5dd9c3b6f |
| origin/t3/booklab | e4d79083b307617691da34fb6c606bd5ffc47847 |
| CheckerBoard reference, unchanged since round 5 | 32f194f840f208f0aff86a07026f788c01c819ce |

All work was under /home/admingod/regicide-review-a. Builds, tests, our DLL calls and referee fixtures ran foreground with nice -n 19. Builds used at most two jobs and every search used one worker; every command finished within ten minutes. Temporary files stayed in this clone. No real opponent engine, production launcher, protected directory, lock or other agent's process was used. Scripted runner fixtures have no search. The API test's short self-play uses our own DLL only.

Executed: a fresh Clang/CMake release build of the fix branch; its API test and runner claims/cbclaims/movelimit/claimflags/pairs/restart modes; before-code failures; the bench; direct DLL fixtures; independent move-generator/referee checks; and the extracted CheckerBoard search/result functions. The source-only reading covers the surrounding GUI path, claim predicates and Book Lab launch recipes. The full Windows GUI was not built or run.

The private two/three-piece WLD/MTC database was built in round 6 with tests/test_egdb.hpp and reused read-only. The baseline DLL and runner are the pre-fix round-6 artifacts. The strict PDNs are newly parsed for scores/comments; unchanged canonical move/FEN replays and first-varying-move tables from round 6 are reused after checking all ten input hashes.

## Finding 1 — a win is claimed although the opponent can complete a third occurrence

File: [src/search.cpp:782–788, Engine::claim](https://github.com/AdminGodZ/Regicide/blob/e252fd0db300776815dc5d32b990a8a5dd9c3b6f/src/search.cpp#L782); the return reaches CheckerBoard through [src/cb_api.cpp:261–265](https://github.com/AdminGodZ/Regicide/blob/e252fd0db300776815dc5d32b990a8a5dd9c3b6f/src/cb_api.cpp#L261).

Set HCE, one thread, hash 1 MB, book and avoid off, and the private three-piece tables. Give the DLL this complete legal history:

```text
set gamehist B:WK8:BK2,K5 5-1 8-3 1-5 3-8 5-1 8-3
```

Then call getmove on B:WK3:BK1,K2 as Black, maxtime 0.01, reset flag 1, playnow 0.

Observed on the newest fix branch: 1-5, score 19994, zero nodes, return CB_WIN (1). The root is its second occurrence, reversible count 6. Our played child is also only its second occurrence, at reversible count 7. White can now play 3-8, producing B:WK8:BK2,K5 for the third time at ply 8. The referee draws it. No position in the supplied history has occurred three times before the tested reply.

The follow-up guard verifies the current root and the immediate child, then treats the MTC distance and the remaining no-progress window as enough evidence for a win claim. Neither distance check rules out this historical repetition on the opponent's reply. CheckerBoard would stop before that reply and award us a win from a move after which the opponent can draw.

This was found with a bounded enumeration of legal four-ply king cycles, then independently replayed and reproduced through getmove; it is not an inference from a score. The first tested cycle already supplies the counterexample. [api-cases-after.tsv](tables/api-cases-after.tsv) contains the actual return, move, info string and referee counters. [future_repeat.cpp](fixtures/future_repeat.cpp) is the small enumerator.

Proposed API regression inside the existing claim-test block, after the three-piece database has loaded:

```cpp
const int reply_draw = claim(
    "B:WK3:BK1,K2",
    "B:WK8:BK2,K5 5-1 8-3 1-5 3-8 5-1 8-3",
    "1-5");
if (reply_draw == 1) return 1; // White's 3-8 draws by repetition.
```

A sound claim needs evidence that includes the relevant earlier positions along the continuation. A board-only MTC result plus the clock guard does not provide that evidence.

## Finding 2 — a terminal win claim at the allowed limit is recorded as unfinished

Files: [tools/match/main.cpp:333 and :350](https://github.com/AdminGodZ/Regicide/blob/e252fd0db300776815dc5d32b990a8a5dd9c3b6f/tools/match/main.cpp#L333), followed by the no-move branch at :203–205.

Input: opening B:W10:B6, concurrency 1, pairs together, --cb-claims --cb-move-limit 1. Both scripted DLLs play the only legal move, 6x15, and return CB_WIN.

Observed on the newest fix branch, both colours/legs:

```text
result *, score_a empty, termination "move limit",
engine_plies 1, first_claim "ply 1 B win"
```

The runner skips claims when the child has no legal move. It starts the next loop, counts the missing move as attempt 2, and replaces the result with unfinished because 2 exceeds the limit of 1.

The pinned CheckerBoard SearchThreadFunc accepts 6x15 and stops on the win return during attempt 1. With maxmovecount 1, the actual update_match_stats writes 1-0. I compiled and ran those functions for this exact position: gameover 1, PDN 1-0. No second attempt occurs there. The runner's missing-move counting is correct when no accepted claim ended the preceding attempt; it must retain that existing test while handling this claim at attempt 1.

The terminal win/draw fixtures without a move limit both retain the intended board win, and the ordinary N+1 draw-claim fixture now correctly produces two unfinished games. The defect is the combination of an accepted terminal win claim with the boundary. Evidence: [runner-cases.log](evidence/runner-cases.log), [terminal-reference.log](evidence/terminal-reference.log), [terminal-win-limit1-results.csv](tables/terminal-win-limit1-results.csv).

Proposed regression using runner_test.cpp's existing claim_run, where A claims and B returns unknown:

```cpp
ClaimRun terminal = claim_run(self, match_exe, "win", 1,
    "--cb-claims --cb-move-limit 1", "B:W10:B6");
// A as Black claims the terminal win on attempt 1.
// B as Black does not claim; finding White without a move is attempt 2.
if (terminal.status != 0 || terminal.games.size() != 2 ||
    terminal.games[0]["Result"] != "1-0" ||
    terminal.games[1]["Result"] != "*") return 1;
```

## Task 1 — CheckerBoard's draw return

Reference function: [CheckerBoard.c, SearchThreadFunc, lines 3567–3587](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L3567). After a valid move is found and added to the game, the GUI checks rule draws at :3573–3575. Otherwise the engine-match branch at :3576–3580 contains:

```cpp
if (game_result == CB_DRAW) {
    gameover = TRUE;
    add_gameover_comment = true;
}
```

Only the other return values pass through the early_game_adjudication test at :3583. Thus one engine's CB_DRAW ends the game with early adjudication on or off. There is no test here for a second engine agreeing, a setting enabling draw claims, piece count or minimum move number. The generic requirements are a found legal move, no abort, and engine-match state. A rule draw already detected has the same drawn outcome.

[update_match_stats, lines 4454–4457](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L4454) increments draws and uses the draw PDN result. The saved English-checkers result is 1/2-1/2. This is a getmove return; enginecommand's numeric success/failure is unrelated.

I extracted and compiled the complete SearchThreadFunc, actual detect_nonconversion_draws and update_match_stats, plus pdn_result_to_string. A portable adapter supplies boards, legal moves, one scripted getmove, UI/thread/logging stubs and result storage. This executes the reference function, rather than a rewritten claim predicate. All cases are the first searched move, below the move limit, with no pre-existing board draw. Material/start-colour controls are B:WK32:BK1, the ordinary 24-piece starting board, and W:WK32:BK1.

| Early adjudication | getmove return | Black mover, 2 pieces | Black mover, 24 pieces | White mover, 2 pieces |
|---|---|---|---|---|
| Off | 0, CB_DRAW | Ends, 1/2-1/2 | Ends, 1/2-1/2 | Ends, 1/2-1/2 |
| Off | 1, CB_WIN | Continues; no result written | Continues; no result written | Continues; no result written |
| Off | 2, CB_LOSS | Continues; no result written | Continues; no result written | Continues; no result written |
| Off | 3, CB_UNKNOWN | Continues; no result written | Continues; no result written | Continues; no result written |
| On | 0, CB_DRAW | Ends, 1/2-1/2 | Ends, 1/2-1/2 | Ends, 1/2-1/2 |
| On | 1, CB_WIN | Ends, 1-0 | Ends, 1-0 | Ends, 0-1 |
| On | 2, CB_LOSS | Ends, 0-1 | Ends, 0-1 | Ends, 1-0 |
| On | 3, CB_UNKNOWN | Continues; no result written | Continues; no result written | Continues; no result written |

Separate qualification: [update_match_stats :4420–4424](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L4420) writes unknown (*) when the attempt count has already exceeded the configured limit, regardless of the return. The table deliberately stays within that limit. It establishes source-revision behaviour, not that a deployed DLL made a particular claim.

Raw output: [checkerboard-claims.tsv](tables/checkerboard-claims.tsv). The [adapter generator](fixtures/reference_adapter.py) extracts the reference functions from the pinned checkout; UI stubs do not decide when a claim stops the game.

## Task 2 — independent claims-census recount

The six cl-* halves arrived at e4d7908, 156 games apiece. I parsed every move comment independently of cl/claims_census.md and replayed all 101,794 moves with the separate referee. Every move is legal; every final board result agrees with the PDN and CSV. Counts from claim=... in the comments agree with ClaimsA/ClaimsB; their earliest event agrees with both FirstClaim and results.csv first_claim. The 936 game identifiers are unique within their run. Sources and decompressed hashes: [census-sources.tsv](tables/census-sources.tsv).

All outcomes below are from Regicide's perspective. Claims were recorded but not acted on during these runs. All 26 decisive games are Regicide wins; there is no opponent draw claim anywhere in a decisive game.

| Opponent | Games | At least one claim | No claim | First-claim ply min / median / max | Board W/D/L | Recorded-game --cb-claims W/D/L | First claim disagrees with board |
|---|---|---|---|---|---|---|---|
| Cake | 312 | 312 | 0 | 26 / 45 / 112 | 1/311/0 | 1/311/0 | 0 |
| KingsRow | 312 | 306 | 6 | 41 / 51 / 137 | 3/309/0 | 3/309/0 | 0 |
| GuiNN | 312 | 311 | 1 | 28 / 43 / 128 | 22/290/0 | 22/290/0 | 0 |

Counts by claiming engine, board colour and kind follow. "Games" means at least one such claim in that game; "First" means it was the first claim of the whole game. A game can occur in more than one kind/engine row. Zero rows are omitted: there are no Regicide draw/loss returns and no opponent win returns in this census.

| Opponent | Claiming engine | Colour | Kind | Games | Claim events | First |
|---|---|---|---|---|---|---|
| cake | Regicide | B | win | 1 | 41 | 1 |
| cake | cake | B | draw | 156 | 2693 | 156 |
| cake | cake | W | draw | 155 | 3289 | 155 |
| cake | cake | W | loss | 1 | 34 | 0 |
| guinn | Regicide | B | win | 4 | 212 | 4 |
| guinn | Regicide | W | win | 18 | 943 | 12 |
| guinn | guinn | B | draw | 138 | 2382 | 138 |
| guinn | guinn | B | loss | 18 | 1188 | 6 |
| guinn | guinn | W | draw | 151 | 2856 | 151 |
| guinn | guinn | W | loss | 4 | 216 | 0 |
| kingsrow | Regicide | W | win | 3 | 158 | 1 |
| kingsrow | kingsrow | B | draw | 153 | 7474 | 153 |
| kingsrow | kingsrow | B | loss | 3 | 211 | 2 |
| kingsrow | kingsrow | W | draw | 150 | 7382 | 150 |


The lead's first-half observation is confirmed: cl-kingsrow-h1 has 150 draw-claim games, two first loss claims and four games without a claim. Its board outcome is 2/154/0. The other half has 153 first draw claims, one first Regicide win claim and two games without a claim, board outcome 1/155/0. Whole-census first claims are: Cake 311 opponent draws + 1 Regicide win; KingsRow 303 opponent draws + 2 opponent losses + 1 Regicide win; GuiNN 289 opponent draws + 6 opponent losses + 16 Regicide wins.

First-claim distribution (games without a claim excluded):

| Opponent | Ply 1–20 | 21–40 | 41–60 | 61–80 | 81–100 | Above 100 |
|---|---|---|---|---|---|---|
| Cake | 0 | 87 | 196 | 20 | 5 | 4 |
| KingsRow | 0 | 0 | 262 | 38 | 3 | 3 |
| GuiNN | 0 | 104 | 184 | 13 | 3 | 7 |

[census-first-ply.tsv](tables/census-first-ply.tsv) gives the exact ply histogram, also split by actor, colour and kind. The seven no-claim games are drawn. There are zero first-claim/result disagreements, so the requested disagreement list is empty; [census-disagreements.tsv](tables/census-disagreements.tsv) contains only its header.

All decisive games are listed below. "First" is the earliest getmove claim of either engine, interpreted from the claimer's colour; "CB result" is our result and ending ply if --cb-claims is applied to this recorded sequence, without adding a move limit. The board's own result on the same move has priority. Each row remains a win. Game numbers here are local to the 156-game half, not full-deck numbers; Round remains available in the TSV.

| Opponent | Half | Game / Round | ACF | Our colour | Final board plies | First claim | Claiming engine | CB result @ ply |
|---|---|---|---|---|---|---|---|---|
| cake | h2 | 101 / 51.1 | 143 | B | 169 | ply 89 B win | Regicide | W @ 89 |
| kingsrow | h1 | 28 / 14.2 | 16 | W | 244 | ply 59 B loss | kingsrow | W @ 59 |
| kingsrow | h1 | 78 / 39.2 | 43 | W | 268 | ply 137 B loss | kingsrow | W @ 137 |
| kingsrow | h2 | 86 / 43.2 | 134 | W | 184 | ply 72 W win | Regicide | W @ 72 |
| guinn | h1 | 8 / 4.2 | 5 | W | 200 | ply 50 W win | Regicide | W @ 50 |
| guinn | h1 | 10 / 5.2 | 6 | W | 152 | ply 42 W win | Regicide | W @ 42 |
| guinn | h1 | 16 / 8.2 | 9 | W | 158 | ply 35 B loss | guinn | W @ 35 |
| guinn | h1 | 28 / 14.2 | 16 | W | 154 | ply 40 W win | Regicide | W @ 40 |
| guinn | h1 | 39 / 20.1 | 22 | B | 145 | ply 55 B win | Regicide | W @ 55 |
| guinn | h1 | 46 / 23.2 | 25 | W | 230 | ply 57 B loss | guinn | W @ 57 |
| guinn | h1 | 48 / 24.2 | 27 | W | 234 | ply 128 W win | Regicide | W @ 128 |
| guinn | h1 | 55 / 28.1 | 31 | B | 237 | ply 119 B win | Regicide | W @ 119 |
| guinn | h1 | 62 / 31.2 | 34 | W | 148 | ply 36 W win | Regicide | W @ 36 |
| guinn | h1 | 78 / 39.2 | 43 | W | 234 | ply 63 B loss | guinn | W @ 63 |
| guinn | h1 | 112 / 56.2 | 64 | W | 202 | ply 31 B loss | guinn | W @ 31 |
| guinn | h1 | 114 / 57.2 | 65 | W | 240 | ply 34 W win | Regicide | W @ 34 |
| guinn | h1 | 132 / 66.2 | 74 | W | 200 | ply 58 W win | Regicide | W @ 58 |
| guinn | h2 | 4 / 2.2 | 90 | W | 178 | ply 51 B loss | guinn | W @ 51 |
| guinn | h2 | 24 / 12.2 | 101 | W | 170 | ply 58 W win | Regicide | W @ 58 |
| guinn | h2 | 43 / 22.1 | 112 | B | 161 | ply 51 B win | Regicide | W @ 51 |
| guinn | h2 | 80 / 40.2 | 131 | W | 204 | ply 38 W win | Regicide | W @ 38 |
| guinn | h2 | 84 / 42.2 | 133 | W | 194 | ply 90 W win | Regicide | W @ 90 |
| guinn | h2 | 98 / 49.2 | 141 | W | 184 | ply 28 W win | Regicide | W @ 28 |
| guinn | h2 | 106 / 53.2 | 146 | W | 162 | ply 61 B loss | guinn | W @ 61 |
| guinn | h2 | 112 / 56.2 | 149 | W | 154 | ply 36 W win | Regicide | W @ 36 |
| guinn | h2 | 119 / 60.1 | 153 | B | 181 | ply 49 B win | Regicide | W @ 49 |


[census-decisions.tsv](tables/census-decisions.tsv) also gives both process ages, worker, full opening name, first claim by each engine and actual PDN results. [census-games.tsv](tables/census-games.tsv) provides the same audit fields for all 936 games, including first-claim root FEN, piece count and reversible count.

This agrees with the committed claims_census.md's totals and decisive-game claim ordering. It establishes agreement with the games actually played out, not a proof that each claimed board is objectively won/drawn. Nor is it a prediction of a new kept-process run: early endings change the subsequent processes' histories and scheduling. The three actual --cb-claims concurrency-1 mo2 runs remain separate data and are not substituted for these counterfactuals.

## Task 3 — drawn near misses in the strict records

Inputs: mo-cake-{a,b}-{h1,h2}, mo-kingsrow-{a,b}-{h1,h2}, mo1-cake and mo1-kingsrow. There are 1,872 games, 1,869 drawn. All ten decompressed PDN hashes match the independently replayed round-6 inputs; [record-sources.tsv](tables/record-sources.tsv) records their counts, tags and hashes.

I parsed every Regicide move comment after the three imposed ballot plies. No own move lacks a node count, no searched own move lacks a score, and every game has an own scored search. There are 13 qualifying drawn games, from ten distinct opponent/opening/colour contexts. All are Regicide Black. Three meet the first-search threshold, twelve the later-search threshold, with two meeting both. The scored values are the engine's reported opinions; no search or claim of an objective loss is added by this list.

Definitions:

- A searched move has nodes > 0. The first such own move marks the observed end of the initial book block. All 13 first-search positions have 12–22 pieces, above the database range; the preceding own move has zero nodes. The table gives last zero-node own ply → first searched own ply (its score). This measures the boundary visible in the comments, not internal membership in the book file.
- A game qualifies if that first search is <= -40, or a later own searched move is <= -60. Minimum score uses own searched moves only, and its earliest ply in a tie. Later zero-node book re-entries are not treated as searches.
- "First varying" reuses the round-6 opponent-choice table: the earliest opponent decision with alternatives among games arriving through the same sequence. An earlier variation need not make this game's entire line unique.
- "First strict departure" compares against every other strict game with the same opponent, opening, our colour and starting board, excluding this game. It is the first edge absent from all peers still sharing its exact prefix. All 13 departures are opponent moves. This supplies a stricter interpretation of leaving known lines; the 8-worker records do not establish a chronological first occurrence.
- Ages A/B are games already played by the Regicide/opponent process at the start, read from the tags. Worker and age are not inferred from output-file order. Game number is the within-run scheduled number, derived from Round's pair/leg.

Run abbreviations below: C1 = mo1-cake; CA1/CA2/CB1/CB2 = mo-cake-a-h1/a-h2/b-h1/b-h2; KA1/KA2/KB1/KB2 = the corresponding mo-kingsrow runs. Every result in this table is a draw.

| # | Opponent / ACF | Run | Game | Worker | Age A/B | Last book → first search (score) | First varying opponent move | First strict departure | Minimum score @ ply |
|---|---|---|---|---|---|---|---|---|---|
| 1 | cake / 142 | C1 | 255 | 0 | 254/254 | 23 → 25 (0) | 18: 31-27 | 20: 29-25 | -136 @ 37 |
| 2 | cake / 43 | CA1 | 77 | 7 | 10/10 | 29 → 31 (-8) | 24: 32-27 | 48: 13-9 | -108 @ 45 |
| 3 | cake / 43 | CB1 | 77 | 4 | 10/10 | 29 → 31 (-8) | 24: 31-27 | 24: 31-27 | -108 @ 45 |
| 4 | cake / 43 | C1 | 77 | 0 | 76/76 | 29 → 31 (-8) | 24: 32-27 | 48: 6-2 | -108 @ 45 |
| 5 | cake / 8 | C1 | 13 | 0 | 12/12 | 11 → 13 (0) | 12: 29-25 | 12: 29-25 | -90 @ 27 |
| 6 | kingsrow / 36 | KA1 | 63 | 7 | 6/6 | 45 → 47 (0) | 52: 19-15 | 52: 19-15 | -87 @ 55 |
| 7 | cake / 157 | CA2 | 125 | 0 | 12/12 | 23 → 25 (0) | 12: 26-23 | 16: 32-27 | -79 @ 29 |
| 8 | kingsrow / 6 | KA1 | 9 | 5 | 0/0 | 53 → 55 (-78) | 50: 7-3 | 58: 5-9 | -78 @ 55 |
| 9 | kingsrow / 6 | KB1 | 9 | 4 | 0/0 | 53 → 55 (-78) | 50: 7-3 | 58: 15-11 | -78 @ 55 |
| 10 | cake / 152 | C1 | 273 | 0 | 272/272 | 11 → 13 (-28) | 12: 22-17 | 12: 22-17 | -74 @ 33 |
| 11 | cake / 102 | C1 | 181 | 0 | 180/180 | 15 → 17 (0) | 12: 17-14 | 16: 27-24 | -68 @ 29 |
| 12 | cake / 166 | C1 | 299 | 0 | 298/298 | 25 → 27 (0) | 18: 25-22 | 18: 25-22 | -60 @ 39 |
| 13 | kingsrow / 98 | KB2 | 17 | 3 | 2/2 | 29 → 31 (-56) | 30: 17-14 | 30: 17-14 | -56 @ 31 |

The full [near-miss-games.tsv](tables/near-miss-games.tsv) adds the FEN at the minimum score, the first-varying decision and strict departure, plus the peers' alternative moves. [near-miss-summary.tsv](tables/near-miss-summary.tsv) includes zero-count runs, and [near_miss.py](fixtures/near_miss.py) records the filter and grouping. That script takes the local round-6 canonical replay cache and first-varying tables as inputs; the report tables and source manifest are the portable evidence.

ACF 43 illustrates the distinction: CA1 and C1 first differ at opponent ply 24 among the wider records, but their complete strict lines remain shared until ply 48, where Cake selects 13-9 and 6-2 respectively. CB1 already departs from all strict peers at ply 24 with 31-27. All three later show -108 at our ply 45.

Control outside the drawn list: mo1-kingsrow game 9, ACF 6, Regicide Black, ages 8/8, is the recorded loss. Its first departure is exactly opponent ply 50, 19-15 instead of the peers' 7-3, at W:W5,7,14,19,30,32:B4,8,12,20,21,K25. Last initial book move is our ply 49; first search is ply 51, score -79. Its minimum is -19999 at ply 129. The two drawn ACF 6 games above have ages 0/0 and first search ply 55 at -78; their KingsRow ply-50 move is 7-3. [known-loss-control.tsv](tables/known-loss-control.tsv) contains the complete control row.

These are places to test process age, not evidence that age caused the score or that changing age will change a result. In particular, Cake ACF 43 has the same minimum at ages 10 and 76, while the deepest listed near miss, ACF 142, occurs at age 254.

## Task 4 — original fixtures and the remaining claim cases

Freshly built branch e252fd0 (the follow-up to 80bcfb40 requested at 12:58 CET), release, LTO off, Clang, at most two build jobs. The public tests use our DLL and scripted engines only.

| Check | Observed on fix branch | Before-fix control |
|---|---|---|
| Ordinary short Black win, B:W10:B6 | 6x15, CB_WIN 1 | CB_WIN 1 |
| Ordinary short White win, W:W27:B23 | 27x18, CB_WIN 1 | CB_WIN 1 |
| Fresh three-piece win, B:WK3:BK1,K2 | 1-5, 19994, zero nodes, CB_WIN 1 | CB_WIN 1 |
| Same table win after the legal 77-ply round-6 history | 1-5, CB_UNKNOWN 3; no false win claim | CB_WIN 1 |
| Nine-piece White root already third occurrence | 18-14, CB_UNKNOWN 3 | CB_WIN 1 |
| One ply earlier: our 17-13 completes third occurrence | CB_DRAW 0, referee third occurrence | CB_UNKNOWN 3 |
| Only one prior cycle: 17-13 makes the second occurrence | CB_UNKNOWN 3, referee continues | CB_UNKNOWN 3 |
| Nine-piece legal history, root reversible count 78 | 30-25, CB_DRAW 0; child count 79 | CB_UNKNOWN 3 |
| Same history with its first reversible ply removed, root count 77 | CB_UNKNOWN 3; child count 78 | CB_UNKNOWN 3 |
| Draw claim on attempt N+1 with --cb-move-limit N=3 | Both games *, blank score | White claimant's game was scored a draw |
| --cb-claims with --accept-draw-claims | Refused, exit 2, no output directory | Combination accepted |
| Terminal capture with a draw claim, --cb-claims alone | Board win, 1-0 | Board win, 1-0 |
| Default runner, no new claim/limit option | Eight scripted games match before-code result fields and claim metadata | Baseline |

The round-6-fix version of cb_api_test, runner movelimit and runner claimflags tests fail on their preceding code. On the branch, cb_api_test and runner claims, cbclaims, movelimit, claimflags, pairs and restart modes pass. The separate exact boundary fixture in finding 2 exposes a case those passing tests omit.

The bench gives exactly 7,833,556 nodes. All 17 position/move/score/node rows also equal the earlier master baseline, not just the total. The elapsed time is not used as an equality check.

Draw-return predicate: Engine::draws_at_once is the existing root rule helper moved into a method and reused by Engine::claim. It first excludes a move leaving the opponent without a legal reply. Otherwise a child reversible count >=79 draws, or two earlier equal keys in the last child_rev-1 positions before the root complete a third occurrence. The key includes the side to move; the root has the opposite side from the child. Conversion resets the counter/window.

I compared this predicate to a separate referee built on tools/common/checkers.cpp, across all legal candidates of the two long round-6 histories and 300 seeded walks from the starting board, a nine-king board and a two-king board. Result: 11,171 roots, 54,516 candidate moves, including 169 draw returns, 25,778 converting candidates and 261 terminal candidates; zero disagreements. The all-candidate checks also include histories past an already completed draw to exercise the predicate, so these counts are test coverage rather than played-game totals. The independent API cases exercise normal un-stopped calls and the exact second/third and 78/79 boundaries. No false CB_DRAW was found. This is a bounded test, not an exhaustive proof.

The guard still does not establish soundness of every CB_WIN: finding 1 supplies a legal short-history counterexample while preserving the requested ordinary short wins. The stopped-call fallback assigned to reviewer B was not re-reviewed or re-reported.

The e252fd0 follow-up changes the treatment of searched table wins without an exact conversion distance. For roots outside the tables, its predicate is root_rev + (20000 - score) < 79 / 2, with integer division giving 39. The independent four-piece fixture B:WK10,K30:BK1,K6 has the forced 6x15 and score 19999; it is claimed at reversible counts 4 and 37, and refused at count 38. Thus the strict boundary is exercised on either side (37 + 1 < 39; 38 + 1 == 39). Its played capture resets the referee's count to zero, as expected. The new public API test also covers 4 versus 38.

The change is an explicit policy to accept a board-table win when at least half the window remains, rather than a proof of conversion before the limit; its source comment now says so. I do not report that stated policy alone as a defect. Finding 1 is a concrete legal-history draw available against a distance-bearing table move, and remains on this follow-up commit. Roots inside the WLD tables use the conversion-search distance rule; ordinary mate scores use their mate distance. The bound changes do not change search node counts.

## Evidence and reproduction

- [api-cases-before.tsv](tables/api-cases-before.tsv), [api-cases-after.tsv](tables/api-cases-after.tsv): exact API returns, moves, info strings and independent board counters.
- [claim-bench.log](evidence/claim-bench.log), [claim-api-test.log](evidence/claim-api-test.log), [draw_guard-final.log](evidence/draw_guard-final.log), [runner-cases.log](evidence/runner-cases.log): execution results.
- [api_cases.py](fixtures/api_cases.py), [draw_guard.cpp](fixtures/draw_guard.cpp), [future_repeat.cpp](fixtures/future_repeat.cpp), [runner_cases.py](fixtures/runner_cases.py), [census_referee.cpp](fixtures/census_referee.cpp): fixtures, under the review clone's reports/round7 layout. They refer to the private round-6 database/history files; those histories used by the cases are included in fixtures/histories.
- [reference_adapter.py](fixtures/reference_adapter.py): source extractor and adapter generator. CHECKERBOARD_SRC can point to the pinned reference checkout; REVIEW_OUT can select the generated-file directory. Compile the generated reference_adapter.cpp with tools/common/checkers.cpp and tools/common on the include path using nice -n 19 g++ -O2 -std=c++20 -Wno-format-security. The [terminal_reference.py](fixtures/terminal_reference.py) fixture uses the same extracted functions with B:W10:B6, return CB_WIN, early on, movecount=maxmovecount=1.
- [source-heads.txt](evidence/source-heads.txt): final fetch revisions and record-availability check. All committed files in this round are text, below 1 MB.

The [claims_census.py](fixtures/claims_census.py) parser uses [ballots.tsv](fixtures/ballots.tsv) for opening names and the independent census_referee for all board states.

No engine evaluation was run for the records work. The census counterfactuals use the newly committed claim records; they do not estimate a newly played run.
