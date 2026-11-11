# Review A — Round 10

No new code or book defect found. The stated list of 82 old-line entries is not committed: the available records identify 77, and all 77 pass the reachability check. The other five cannot be checked individually without their identities. This is an evidence gap, not a finding that those five are reachable.

Reviewed master **7f3ac697bc6dc69630b3b55bad92ad00e428aa4c** and Book Lab **7ca6c27198ea0cc89f315744448fdc005ea81423**, followed by **bfce8b08a4d186e9b198cc80e437ecb42e44ec35** when the mo3 records arrived. The ledger on origin/master was read first and checked again at publication. Earlier accepted items are not reported again.

All commands ran in `/home/admingod/regicide-review-a`. Compilers, drivers, record parsers and engine calls ran at nice 19, with one search worker, no concurrent engine searches, and foreground timeouts below ten minutes. No opponent engine was run. The shared `/home/admingod/regicide-data/egdb` was used read-only, with a 16 MiB pre-fault budget; it provides complete WLD through eight pieces and MTC through six. No other job directories, processes or locks were touched. Only the text files in this report directory are committed.

## 1. Candidate r11f

The committed candidate is 1,413,600 bytes and has SHA-256 **a4c90ab6ff083480ad9858fd2a1217b3fb4427dab6539ecbe13546855b0e188f**, exactly the requested payload. The r11d comparison is the committed `wp11d/book_r11d.bin`.

The actual checks in [book_audit.cpp](book_audit.cpp) give:

| Check | Result |
| --- | --- |
| Book entries checked against engine and independent referee legal moves | 88,344; all legal |
| Avoid entries | 5; all legal; byte-identical to r11d |
| Entries without an identified full position | 0 |
| Distinct forced decisions checked against the output book | 54,261 |
| Positions with two different forced decisions | 0 |
| Changed existing decision | Exactly one: 6-10 replaced by 8-11 |
| Added decisions | Exactly 149 |
| Other score/depth/count changes or removed moves | None |
| Requested engine probes | All 149 new pins plus the changed decision: expected move, depth 0, nodes 0 in all 150 calls |

The changed position is `B:W19,21,22,23,27,28,30,31,32:B2,4,5,6,7,8,12,13,14`. The entry-level diff has 150 added records and one removed record: one addition/removal pair is that replacement, and the remaining 149 are new pins. [book-diff.tsv](tables/book-diff.tsv) contains every difference. Forced decisions have no special score/depth marker in this format; the check reads all force/force-unavoid decisions and verifies that each output position has exactly its specified move.

For legality, positions were recovered from the decisions, independent record replay, and legal expansions through the underlying book. Three otherwise missing avoid positions were recovered from committed loss/avoid experiment text, then checked with the referee. The recovered FENs are in [avoid-fens.txt](avoid-fens.txt). The audit identifies 824,221 distinct positions and replays 24,656 committed games / 2,652,707 legal plies. The broad input starts with Book Lab 7ca6c27 and includes the later mo3 PDNs appended without changing the frozen sample IDs. The later commit changes none of the candidate, decisions, pin sources or dip inputs.

Every new pin's exact position/move occurs in the two specified w14c6 KingsRow source runs. Those 48 games were replayed independently to their board draws: 24 fresh and 24 kept. There are no lost or unknown source games. The 150 requested decisions have 999 matching source occurrences in total, from one to 48 per decision, all draws. [pin-provenance.tsv](tables/pin-provenance.tsv) gives each FEN, move, W/D/L occurrence count and actual engine return. The engine test installs the candidate through `Engine::set_book(..., 0)` and its avoid list; it exercises the normal `Engine::go` book-return path, using the current master's core and built-in n8q evaluator.

### Reachability and the stated 82

The build script's final Python block infers “unreachable” by subtracting positions seen in other recorded games from positions seen after the old 6-10. It prints a count, without the FEN list, and does not prove reachability from the deck. The separate 82 in `pins_r11f_new.md` means source positions skipped during pinning because they were inside king-move runs; it is not a list of the old-line entries.

Reconstructing the script's old-line sets from all 208 committed PDN files gives **77** old-only entries, not 82. Each of those 77 is listed in [old-line-reach.tsv](tables/old-line-reach.tsv). None is reached under r11f from any of the 156 ballots, with either Regicide colour, even when the opponent may choose **every legal reply**. This includes checking arrival at the position before deciding whether the book can be consulted there. The count 82 and its missing five identities remain unverified. The lead was asked for the original list during the round.

The stronger graph check starts at the official three-move ballot positions. At each Regicide turn it takes r11f's actual margin-0 move, subject to the engine's book eligibility (no reversible king run and no database-decided root); at opponent turns it explores every legal reply. A second pass restricts opponent replies to moves observed at the identical position in the committed records, across all opponents and runs. It does not require the graph path to occur in one single recorded game. Full-position keys, side to move and reversible count are tracked. A line stops when our next move requires a search.

For the changed decision and 149 pins:

- **No position is reachable from another ballot**, even with every legal opponent reply.
- All reachable origins are **ACF #6 Inferno, Regicide Black**.
- **86 of 150** are reached at a turn where the candidate book is consulted: the changed decision and 85 new pins.
- **87 of 150** positions are reached at all; one additional pin is reached with a nonzero reversible count, so it is not consulted there. The remaining 63 positions have no such path under the final candidate.
- Restricting opponent replies to observed record moves gives the same target counts and origins.

The unused pins are legal stored decisions; forcing one choice upstream can make other source-game branches unused. [target-reach.tsv](tables/target-reach.tsv) gives every target and both graph results. Across the complete graph, 37 previously consultable r11d positions cease to be consultable under r11f; [all-unreachable.tsv](tables/all-unreachable.tsv) lists them. That is a different quantity from “entries seen only after 6-10 in the source games”: some of the 77 were already unused by the final r11d choices.

## 2. Table-drawn roots, before and after PR #81

I used actual committed game histories and the permitted eight-piece shared tables, rather than generated three-piece substitutes. Eligible inputs have Regicide to move, at most eight pieces, an own comment with positive nodes, and a WLD-drawn root. There are 239,739 distinct candidate FENs in the frozen record input. The sample seed is **0xA10F81**; 300 distinct FENs were selected, stratified by piece count, preserving the full history from each source game.

| Pieces | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| Roots | 43 | 43 | 43 | 42 | 43 | 43 | 43 |

There are 201 Black roots and 99 White roots. The before core is **fdd154e**, the after core **7f3ac69**, both with the same built-in n8q net, hash 128 MiB, one worker, book and avoid list disabled, a fresh hash/history per root, and the same full game history. The reused Round 8/9 libraries were verified against these revisions: the relevant `src`/CMake inputs are byte-identical; no engine source was patched for the comparison.

**300/300 return exactly the same move and score. Every score is 0. There are no differing cases to list.**

The node limit was 1,000,000 per call, with no time limit. These are table-drawn searches: each reaches the maximum completed depth **127** before spending that budget. Actual work was **188,309 nodes before**, **129,540 after**, over all 300 calls (per-root ranges 254–1,778 and 254–1,651). None returned a partial iteration. Thus this is a completed-search comparison under identical 1M limits, not a claim that each root consumed 1M nodes.

The root list filter reduces work on losing children without changing the played move in this sample. This is a bounded sample, not a proof for all positions. [draw-comparison.tsv](tables/draw-comparison.tsv) contains every root, record identity, reversible count, move, score, completed depth and actual nodes. [draw-requests.tsv](draw-requests.tsv) preserves the histories; [budget.cpp](budget.cpp) is the unmodified-core driver.

## 3. Validation and mo3 records

The new mo3 PDNs arrived at bfce8b0 during the round. I independently replayed all three runs, the three v11f arms and the two pin-source runs (48 games in total): **1,080 games, 65,185 plies**. PDN/result CSV agreement, unique game numbers, result perspective, engine-ply counts, FirstClaim, per-engine claim totals, terminal board rules and process-age tags were checked. [validation-sources.tsv](tables/validation-sources.tsv) records decompressed PDN hashes. [record-games.tsv](tables/record-games.tsv) gives the complete game-level recount.

| Run | Games | W | D | L | Unknown |
| --- | --- | --- | --- | --- | --- |
| v11f-c6-fresh | 24 | 0 | 24 | 0 | 0 |
| v11f-c6-kept | 24 | 0 | 24 | 0 | 0 |
| v11f-c6-natural | 48 | 0 | 48 | 0 | 0 |
| mo3-cake | 312 | 0 | 312 | 0 | 0 |
| mo3-kingsrow | 312 | 1 | 311 | 0 | 0 |
| mo3-guinn | 312 | 23 | 289 | 0 | 0 |

The fresh/kept/natural validation runs ignore claims and continue to board draws. Fresh has 21 repetitions and three no-progress endings; kept 12 and 12; natural 21 and 27. Fresh and kept are 24 Black games each; natural is 24 Black plus 24 White games. The 24 fresh games all start both processes at age 0; kept has eight games each at ages 0, 1 and 2; natural has eight each at ages 0 through 5. The natural Black games occur at even ages and their White partners at the following odd ages.

The mo3 ending breakdown, with claims scored from the claimer's colour, is:

| Opponent | Opponent draw claim | Opponent loss claim (our win) | Our win claim | Board repetition | Limit / unknown |
| --- | --- | --- | --- | --- | --- |
| Cake | 312 | 0 | 0 | 0 | 0 |
| KingsRow | 304 | 1 | 0 | 7 | 0 |
| GuiNN | 288 | 8 | 15 | 1 | 0 |

All mo3 processes are Worker 0, with EngineGamesA = EngineGamesB = game number minus one, verified in all 936 games. Every claim ending occurs at the recorded terminal ply and implies the recorded result; every repetition ending is a referee draw, with no earlier board-ending condition. GuiNN game 104 also carries our draw return at ply 50, but its termination is board repetition because the board rule is checked before the claim. These records do not continue past claim endings, so they do not establish the eventual board result of those games.

Every decisive game is listed below. Both process ages are the number of games already played before this game, not the age at the final move. There are no losses or unknowns.

| Opponent / game | Opening | Our colour | Result | Age A | Age B | End / total ply |
| --- | --- | --- | --- | --- | --- | --- |
| kingsrow 28 | ACF #16 Wilderness II | W | W | 27 | 27 | loss claim / 59 |
| guinn 8 | ACF #5 Dreaded Edinburgh | W | W | 7 | 7 | win claim / 40 |
| guinn 10 | ACF #6 Inferno | W | W | 9 | 9 | win claim / 42 |
| guinn 16 | ACF #9 Wilderness I | W | W | 15 | 15 | win claim / 44 |
| guinn 28 | ACF #16 Wilderness II | W | W | 27 | 27 | win claim / 84 |
| guinn 49 | ACF #28 | B | W | 48 | 48 | win claim / 55 |
| guinn 58 | ACF #32 | W | W | 57 | 57 | win claim / 44 |
| guinn 62 | ACF #34 Twilight Zone | W | W | 61 | 61 | loss claim / 45 |
| guinn 78 | ACF #43 | W | W | 77 | 77 | win claim / 64 |
| guinn 82 | ACF #47 | W | W | 81 | 81 | loss claim / 51 |
| guinn 86 | ACF #50 | W | W | 85 | 85 | win claim / 92 |
| guinn 116 | ACF #66 | W | W | 115 | 115 | loss claim / 85 |
| guinn 126 | ACF #71 Diabolical Denny | W | W | 125 | 125 | loss claim / 37 |
| guinn 132 | ACF #74 | W | W | 131 | 131 | win claim / 62 |
| guinn 154 | ACF #86 Octopus | W | W | 153 | 153 | win claim / 40 |
| guinn 162 | ACF #91 | W | W | 161 | 161 | win claim / 72 |
| guinn 180 | ACF #101 | W | W | 179 | 179 | win claim / 58 |
| guinn 199 | ACF #112 | B | W | 198 | 198 | loss claim / 58 |
| guinn 205 | ACF #116 | B | W | 204 | 204 | win claim / 47 |
| guinn 240 | ACF #133 Octopus | W | W | 239 | 239 | loss claim / 143 |
| guinn 254 | ACF #141 | W | W | 253 | 253 | win claim / 44 |
| guinn 262 | ACF #146 | W | W | 261 | 261 | loss claim / 63 |
| guinn 274 | ACF #152 | W | W | 273 | 273 | loss claim / 57 |
| guinn 275 | ACF #153 Black Widow | B | W | 274 | 274 | win claim / 69 |

### ACF #6, Regicide Black

All three games are game 9, round 5.1, both process ages **8**. All finish drawn. The full lines, first searched moves and metadata are in [acf6-black.tsv](tables/acf6-black.tsv), including every Black game of the repeated-opening validation and the pin sources.

| Opponent | First searched ply | Move | Score | Depth | Nodes |
| --- | --- | --- | --- | --- | --- |
| cake | 31 | 9x18 | 0 | 25 | 3197211 |
| kingsrow | 61 | 12-16 | 0 | 28 | 4864205 |
| guinn | 43 | 32-27 | 0 | 31 | 5743247 |

KingsRow follows the repaired position: ply 17 **8-11**, 18 **28-24**, 19 **6-9**, 20 **22-18**, 21 **7-10**, 22 **24-20**, 23 **13-17**, 24 **20-16**, 25 **11x20**. The first searched move is ply 61, **12-16, score 0**. Its complete line is:

```
9-13 22-18 10-14 18x9 5x14 25-22 11-15 22-18 15x22 26x17x10 7x14 29-25 3-7 25-22 1-5 24-19 8-11 28-24 6-9 22-18 7-10 24-20 13-17 20-16 11x20 18-15 17-22 15x6 22-25 6-1 9-13 19-15 4-8 23-19 25-29 27-24 20x27 31x24 29-25 24-20 25-22 20-16 22-18 16-11 13-17 11x4 18x11 30-25 14-18 21x14 18-23 25-21 23-26 14-10 26-31 10-7 11-16 7-3 16x23 4-8 12-16 8-11 16-20 3-7 31-26 11-16 2x11 16x7 26-22 1-6 23-18 6-10
```

Cake and GuiNN choose 26-22 at ply 6 and avoid the repaired ply-17 position; they play 8x15 at ply 17. Their first own searches are respectively ply 31 (9x18, 0) and ply 43 (32-27, 0). The candidate's changed decision is therefore exercised in the KingsRow game, not all three games bearing the ACF 6 label.

## 4. WP14 dip test set

All 13 histories independently replay to the exact committed FEN, with the stated side to move, legal move count and move played. Their game score/depth matches the identified source game's own comment. Every absolute game-score / recorded-100M-score difference is at least 50. There are **12 distinct FENs**: rows 2 and 3 transpose through different ACF 43 histories. Both histories are retained and return identical fixed-node results.

Each row is searched separately from its full history with current master **7f3ac69**, the built-in n8q net, book and avoid list off, hash 128 MiB cleared before each call, one worker, WLD eight/MTC six, no time limit, and node budgets 1M, 3M and 10M. The actual node polling overshoots the requested budget by 321–450 nodes, depending on budget/case; [dip-results.tsv](tables/dip-results.tsv) records actual nodes, moves, depth, partial flag and completed score. All 39 calls end with a completed score and no partial flag. The 100M column below is the committed reference, **not rerun in this review**.

| Row | Source game | Ply | Game score | 1M | 3M | 10M | 100M record |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | mo1-cake:128.1 | 37 | -136 | 0 | 0 | 0 | 0 |
| 2 | mo-cake-a-h1:39.1 | 45 | -108 | 0 | 0 | 0 | 0 |
| 3 | mo-cake-b-h1:39.1 | 45 | -108 | 0 | 0 | 0 | 0 |
| 4 | w14c6-kingsrow-fresh:1.1 | 25 | -108 | -34 | -20 | -15 | -2 |
| 5 | mo1-cake:7.1 | 27 | -90 | 0 | 0 | 0 | 0 |
| 6 | mo-kingsrow-a-h1:32.1 | 55 | -87 | 0 | 0 | 0 | 0 |
| 7 | mo-cake-a-h2:63.1 | 29 | -79 | 0 | 0 | 0 | 0 |
| 8 | mo1-cake:137.1 | 33 | -74 | 0 | 0 | 0 | 0 |
| 9 | mo1-cake:91.1 | 29 | -68 | 0 | 0 | 0 | 0 |
| 10 | w14c6-kingsrow-kept:9.1 | 27 | -76 | -51 | -24 | -17 | -8 |
| 11 | mo1-cake:150.1 | 39 | -60 | 0 | 0 | 0 | 0 |
| 12 | mo-kingsrow-b-h2:9.1 | 31 | -56 | 0 | 0 | 0 | 0 |
| 13 | mo1-cake:150.1 | 31 | 69 | 0 | 0 | 0 | 0 |

All thirteen roots have exactly **one legal move**, and all game comments report depth **1**. In the normal timed search, `Worker::iterative` (`src/search.cpp:599`) stops after a completed iteration when only one root move exists and there is a soft/hard time limit. The node-only calls have no such time limit and search further. This explains why the file compares a shallow forced-move report with a much deeper score; it does not demonstrate that the engine chose a worse root move. Eleven rows score 0 at every new budget. The two ACF 6 roots improve with budget, reaching -15 and -17 at 10M; those are search opinions, not proofs of the eventual result.

The histories in [dips-requests.tsv](dips-requests.tsv), the input row hashes in [dip-results.tsv](tables/dip-results.tsv), and the independent referee output [dip-legal.tsv](evidence/dip-legal.tsv) make each test case inspectable. Capture text in the engine result uses endpoints (for example 15x29); the referee gives the complete corresponding path (15x22x29).

## Reproduction and limits

[COMMANDS.md](COMMANDS.md) describes the builds and driver invocations. The saved drivers contain the review checks; no production engine changes were made or committed. The record parser uses committed PDN move comments, never opponent DLL execution. [record_check.py](record_check.py) recounts the validation/mo3 inputs, and [finish_tables.py](finish_tables.py) validates dip source scores and produces the comparison tables.

Read without running: the production `build_r11f.sh` and validation launch recipes. Those scripts reference production work directories and process-control commands, so I did not execute them. The candidate was checked as committed, entry by entry; its production build script was not rerun. Ran: the engine/referee legality and reachability audit, 150 normal engine book probes, 600 paired table-root searches, 39 dip searches, and the independent record replays/counts described above.

The only unresolved requested evidence is the identity of five of the stated 82 old-line entries. The report does not substitute the pin report's separate 82 skipped positions for them.
