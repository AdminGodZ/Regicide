# Review A — Round 6

Date: 2026-11-11. Local report: /home/admingod/regicide-review-a/reports/round6/REPORT.md.

There are four findings: a table win claimed despite the remaining no-progress window, a search win claimed at an already repeated root, a claim scored on the move-limit boundary, and the old/new draw-claim flags overriding a terminal capture. All four were re-checked after the two feature branches merged into master.

The no-loss-claim change itself passes, including its test failing on the code before it. A sound draw claim can matter in the nine-piece repetition example below because of the published CheckerBoard detector's boundary. Numeric score zero is not sufficient evidence to claim a draw.

The remaining E2 and all three full-deck concurrency-1 PDNs arrived during the review. The long runs recount to Cake 0/312/0, KingsRow 1/310/1, GuiNN 21/291/0. All 936 long-run games have both process counters exactly equal to game number minus one, including all 30 requested opening/colour checks. Every decisive game and both measured ages are listed below.

## Sources and execution

Read docs/REVIEW_LEDGER.md on origin/master before work and after the final fetch. Prior accepted/assigned findings are not re-reported. Finding 1 extends beyond PR #65's immediate-draw check: the tested draw occurs on the following opponent move.

| Ref | Revision checked |
|---|---|
| origin/master | 8de39d0, merge of PRs #76 and #77 during this round |
| origin/t3/match-checkerboard | 007e60c5a981c69dff7f8a03bfe79b0f50a6eb1f |
| origin/t3/no-loss-claim | 46cd070cb2fa3e67d86e361eed0c3453ccf652d1 |
| origin/t3/booklab | e2c16559a72eccd17488d2f23c930f5b919a42bf |
| Before both changes | master 12f0e09 |
| CheckerBoard reference | public revision 32f194f840f208f0aff86a07026f788c01c819ce, the Round 5 reference |

Current master's runner file equals the tested match-checkerboard file; its API file equals the tested no-loss-claim file. I recompiled the latest API and reran the claim counterexamples after fetching the merge. Core engine source/weights are byte-identical to my earlier private master build, so that static library was reused; current API frontends and fixtures were freshly compiled.

All builds/tests/engine calls and referee replays ran foreground with nice -n 19. Builds used at most two jobs; searches used one worker. Every command finished within ten minutes. All files, temporary databases and fixtures were under /home/admingod/regicide-review-a, with TMPDIR redirected there. No real opponent engine, protected directory/lock/process, production launcher or shared database was used. The API test's short self-play uses our own DLL only.

The two/three-piece WLD/MTC database was built privately with tests/test_egdb.hpp, one builder thread. Read-only source inspection is distinguished below from executed fixtures. Extracted CheckerBoard functions were compiled with portable board/UI adapters, not the full Windows GUI.

## Finding 1 — a table score can claim a win that the no-progress rule draws

Files: src/search.cpp:739, Engine::egdb_root_move; :288, Worker::search's distance cutoff; src/cb_api.cpp:260, getmove's score-to-claim conversion.

Concrete root:

~~~text
B:WK3:BK1,K2
77 preceding reversible plies, supplied by set gamehist
WLD: Win; MTC: 6
~~~

The complete reproducible history is [gamehist77.txt](evidence/gamehist77.txt). It is a legal 77-ply king-only history with no repeated board/side pair; no earlier repetition or no-progress draw has occurred. It was constructed by legal predecessor moves, then replayed forward.

Executed latest-master getmove with that history, Black to move, book/avoid off, HCE, private three-piece tables:

~~~text
played 1-5
score 19994 = SCORE_DB_WIN - 6
nodes 0, depth 0, partial false
return 1 = CB_WIN
~~~

The root is not won under the game rules. After 1-5, White can play 3-7, reaching 79 reversible plies and drawing before Black's next capture. I also enumerated all root moves and all replies with terminal/no-progress checks; any conversion then reaches an exactly probed smaller ending. Best play returns draw, not win. The MTC-winning children do not force an earlier conversion.

The current guard asks whether the selected move draws **immediately**. It does not check that the table's future conversion occurs before the remaining limit. Thus the immediate PR #65 cases can pass while this claim is still false.

This is also present in the search's distance cutoff. Suppressing the root shortcut with Limits.infinite=true, depth 4, gives the same 19994 after 16 nodes. With MTC disabled for this root, the conversion search gives 2000 after 25 nodes, below the claim threshold. Merely guarding the root shortcut is therefore not a complete fix; the distance/table cutoffs need the same history/rule qualification.

The direct API call still returns CB_WIN if playnow is already set: the fast table path runs before search stopping. That is safe only when the table-based claim itself is sound.

## Finding 2 — the root's completed third occurrence does not prevent a search win claim

Files: src/search.cpp:255, Worker::search only checks rule draws inside !is_root; :742, Engine::go; src/cb_api.cpp:260.

Concrete nine-piece root:

~~~text
W:WK2,K8,K12,K18,K20,K28,K29:BK13,K30
~~~

Start from that same White-to-move FEN and play:

~~~text
28-24 13-17 24-28 17-13 28-24 13-17 24-28 17-13
~~~

The current root is its third occurrence, with the same side to move. The full command is [repetition-before-white.txt](evidence/repetition-before-white.txt).

Executed latest-master getmove, White, no database, book/avoid off, HCE, maxtime 0.1:

~~~text
played 18-14
score 29987, completed depth 14, partial false, nodes 48512
return 1 = CB_WIN
~~~

The position is already drawn by repetition. This is a terminal-search score rather than a table or book score, so a positive threshold alone does not make the claim sound.

This input can be handed over by the published CheckerBoard implementation: its detector excludes the earliest reversible-window boundary, as checked in Round 5. I ran the actual detector on these eight moves: it returns false, and the GUI's match path can make the next call and accept the win claim. This is not a re-report of the GUI boundary; the new issue is the engine claiming a win without checking its current root's completed draw.

The related Black-to-move call, one ply earlier, demonstrates a useful draw claim. With [repetition-before-black.txt](evidence/repetition-before-black.txt) and:

~~~text
B:WK2,K8,K12,K18,K20,K28,K29:BK17,K30
~~~

our normal search plays 17-13, score 0, returns CB_UNKNOWN, and completes the third occurrence above. A CB_DRAW specifically validated from that played move would end the game correctly and save the half point that the subsequent White claim takes under the GUI path. It is not necessary to infer a draw from a heuristic score.

These histories are legal and have no third occurrence before the final Black move. At the Black root the current board has occurred twice, not three times.

## Finding 3 — a result on the last allowed search bypasses the CheckerBoard move limit

Files: tools/match/main.cpp:199 and :344; tools/match/runner_test.cpp, movelimit.

Concrete executed call, using the scripted claim engine:

~~~text
Opening B:WK32:BK1
A claims draw starting at engine ply 4; B never claims
-c 1 --cb-claims --cb-move-limit 3
~~~

Result:

| Leg | Current runner result | Score for A | Engine plies |
|---|---|---:|---:|
| A Black | * / move limit | blank | 4 |
| A White | draw claim | 0.5 | 4 |

The limit check is at the top of the next loop. The claim at ply N+1 returns before it. CheckerBoard's update_match_stats first checks movecount > maxmovecount and records unknown regardless of that last call's result. I compiled the actual reference function with limit 3 and movecount 4: CB_UNKNOWN, CB_DRAW, CB_WIN and CB_LOSS all produce unknown with PDN *.

The existing new movelimit test explicitly expects the second game to be drawn; it therefore encodes this difference rather than detecting it. For the reference policy both games in this fixture are unknown. The same precedence issue applies at the standard limit 200.

The ordinary no-claim case correctly allows N+1 engine searches and then writes * with no score. This finding concerns a result produced on that boundary, not an off-by-one in the search count.

## Finding 4 — combining claim flags changes a terminal capture win to a draw

File: tools/match/main.cpp:355, the legacy draw-claim branch after the new CB branch.

Concrete input:

~~~text
Opening B:W10:B6
Black plays the legal capture 6x15 and returns CB_DRAW
--cb-claims --accept-draw-claims
~~~

White has no pieces or moves after the capture. With --cb-claims alone, both colour-reversed fixture games correctly score Black wins, termination no legal moves. With both flags, both score draws by claim.

The new claim branch correctly declines to override an empty successor move list. The old accept_draw_claims branch then accepts the same draw without that terminal check. A non-capture terminal move avoids this through the existing continue; a terminal capture exposes it. Both public options are accepted together.

This violates the requested legal-move, board-result, then claim ordering. The flags must be reconciled or the terminal result resolved before either claim branch.

## 1. Every source of a root score at the known-win threshold

SCORE_KNOWN_WIN is 15000, SCORE_DB_WIN 20000, and terminal win scores start at 30000 minus ply.

| Route | What reaches getmove | Claim assessment |
|---|---|---|
| Root table win with MTC | 20000 minus root MTC | Board WLD is insufficient with game history; finding 1 is a concrete false win |
| WLD-only root, conversion search | Conversion heuristic lies between 6000 and 14000; a repeated won position may get 2000 | These values alone never claim a win; terminal/table descendants can still cross the threshold |
| Terminal-search win | 30000 minus terminal distance, backed up through search | Sound for an ongoing game if the searched proof/rule context is sound; finding 2 shows a completed root draw is omitted |
| Search hits a known table leaf | Signed 20000 minus ply, optionally minus MTC | Inherits the table/history qualification; not an independent validation of future rule draws |
| TT cutoff/bound or retained completed score | Previously backed-up score; distances adjusted by ply | Carries the original proof assumptions, not new evidence; no independent heuristic-to-win conversion |
| Book move | Stored int16 score copied directly | Not intrinsically certified; current built-in book cannot cross the threshold |
| Partially stopped iteration with a scored winning move | Its completed move score, possibly a lower bound | A genuine winning lower bound needs only one fully searched root move; partial alone does not make it false |
| Stop before a root move is scored | Previous iteration's score, or 0 if none | A prior valid win can remain valid; zero fallback is not a draw proof |

Read-through conclusions:

- Normal HCE/linear and NNUE evaluations are clamped to ±14999. Conversion progress is capped below 15000. No such static value independently creates a win claim.
- Terminal leaves, database leaves and existing backed-up TT scores are the high-score sources in search. The pruning code avoids decisive-score shortcuts in several relevant places; that is not a formal proof of the entire selective search.
- A stopped child is discarded before its score is written into the root move. A positive result from an unfinished iteration must come from a root move that finished, not from the stop return value.
- A winning lower bound remains sufficient even if other root moves were not searched. A losing partial score is different: it covers only the moves searched. Master formerly guarded its loss claim with !partial; the merged no-loss change now removes all nonterminal loss claims.
- Stopping before scoring anything yields 0, not +32000 or a win. If a later iteration stops before scoring anything, a completed previous win can survive.

Executed route checks:

| Fixture | Result |
|---|---|
| B:W10:B6, no tables/book, ordinary search | 6x15, 29999, completed depth 2 |
| Same with WLD conversion and MTC disabled | 6x15, 29999: terminal proof still claims a win |
| Stop after the first completed winning iteration | 6x15, 29999 retained, depth 1 |
| WLD-only late-history root of finding 1 | 2000, below claim threshold |
| Synthetic legal book entry at B:WK32:BK1, 1-5 scored 20000 | Engine::go returns 20000, despite exact WLD Draw |

The synthetic book is a public Book/Engine API fixture, not an entry of our DLL's current built-in book. Current r11d has 88,195 entries, score range -650..191; zero entries at either known-win threshold ([score inventory](evidence/book-score-range.txt)). Thus the current DLL cannot make a book-derived win claim. A future built-in book carrying a heuristic/mislabelled score of 15000 or more would not be protected by provenance checks. I do not report a fabricated external book as a defect in today's built-in data.

No separate false positive caused solely by the partial flag was found. Findings 1 and 2 show why retaining a numerically positive “proof” is safe only after the applicable rule context is validated.

## 2. Draw claims: when they help, and what zero does not prove

We never return CB_DRAW. Table draws and searched draws normally remain CB_UNKNOWN and are played to the referee's ending.

For a table-drawn root, a completed search that actually scores its root moves with table children has a draw-preserving choice. That is the normal case. There is an interruption exception to the premise that it cannot then lose:

~~~text
B:WK30:BK25,K26, exact WLD Draw
normal search: 26-22, score 0, preserves the draw
playnow already set: fallback 25-21, score 0; the child probes Win for White
~~~

Both API calls return CB_UNKNOWN. The stopped fallback has not earned its zero score through a completed move search. It must not be turned into a blanket draw claim. This is an externally stopped-call limitation, not evidence of an ordinary completed table search discarding the draw.

For nine or more pieces, search score zero is also not enough: it can be static evaluation, a stopped fallback, or a search repetition shortcut at a second occurrence. A valid game-rule claim should check the actual history/no-progress count and played move.

The nine-piece example of finding 2 supplies an actual useful rule claim. Black's normally selected 17-13 makes the third occurrence; returning CB_DRAW on that event would secure the result despite the GUI's boundary omission. It is not a claim based only on score zero. This answers the half-point question with an executed normal-search case, not just an artificial table score.

The same Black root has a separate stopped-call failure:

~~~text
B:WK2,K8,K12,K18,K20,K28,K29:BK17,K30
history: repetition-before-black.txt
available draw: 17-13
normal getmove: 17-13, score 0, CB_UNKNOWN
playnow already set: 30-25, fallback score 0, CB_UNKNOWN
White then has 29x22x13, capturing both Black kings
~~~

A claim after the drawing move is sound. Simply claiming on the stopped zero while playing 30-25 is not a safe general policy: that move abandons the drawing resource. Recognizing/retaining the available rule draw as a fallback is the sound improvement.

I also constructed a legal, unique 78-ply reversible history for that same nine-piece board: [gamehist9-78.txt](evidence/gamehist9-78.txt). Normal and immediately stopped calls both play 30-25, but now that king move itself reaches 79 and the referee draws before White captures. A draw claim buys nothing extra in this no-progress case; the completed rule already protects it.

Thus there is a real repetition case where an explicit, history-validated claim matters under this GUI. There is no basis for converting every zero, second-occurrence shortcut, or table-labelled position into CB_DRAW without keeping the played move consistent with the claimed result.

## 3. Review of the two branches

Both branches became available and were merged into master during this round. Findings 3 and 4 remain in that merged source.

### no-loss-claim

Read the entire change: only the nonterminal known-loss return is removed. No legal move still returns CB_LOSS. Positive known wins remain CB_WIN; other searched results return CB_UNKNOWN.

Ran its new cb_api_test against both DLL versions:

- New API: passes, including 200 self-play plies with every returned board validated as a legal successor.
- Before API: fails on the intended check, a searched known loss returning 2 rather than expected 3.

Additional direct calls verify both colours:

| Input | Before return | New/current return |
|---|---:|---:|
| B:W10:B2, searched loss | 2 | 3 |
| W:W31:B23, searched loss | 2 | 3 |
| B:W6,7,9,10,11:B2, no legal move | 2 | 2 |
| W:W31:B22,23,24,26,27, no legal move | 2 | 2 |
| B:W10:B6, immediate win | 1 | 1 |
| W:W27:B23, immediate win | 1 | 1 |

The change is complete for its stated purpose. The two positive-claim findings are independent of removing loss claims and were rerun with the merged API.

### match-checkerboard: claim data and ordering

Ran six committed runner tests: fresh engines, pairs together, restart counters, claims recorded, CB claims, and CB move limit. All pass.

Claim data is always written in PDN tags FirstClaim/ClaimsA/ClaimsB and CSV first_claim, even if claims do not affect the game. Per-move claim text additionally requires --pdn-comments, as documented. Claims are recorded after move validation, before resolving the board, so a claim that loses priority to a board result is still visible.

The first-claim ply includes imposed ballot moves, matching full PDN ply numbering; the engine-move limit excludes them. Executed a three-move ballot with --cb-move-limit 1: two engine calls were made, result *, and FirstClaim correctly says ply 4 W win.

With --cb-claims alone, a legal move is validated, the board's repetition/no-progress result is resolved, a terminal successor retains the board win, then the claim is interpreted. Win/loss claims are from the mover's perspective, not from A's or Black's fixed perspective. The committed tests cover A claiming as Black and White: two A wins for win claims, two A losses for loss claims, two draws for draw claims. Finding 4 covers the interaction with the older draw flag.

The no-claim move-limit case counts N+1 engine searches as the reference does, writes * and a blank score, increments unfinished, and carries on to the other game with the same processes. It does not score that game or add it to pentanomial statistics. A pair with one or both unfinished legs never becomes a completed pair; a scored companion remains in individual-game W/D/L. The limit/claim precedence on the final search is finding 3.

Without new options, I compared old master 12f0e09 and the new runner on eight controlled games: draw, win, loss and unknown returns, each in both legs. Original result, score, termination and engine-ply fields are identical; all eight end as the same repetition draws. Only the new claim metadata is added. Source changes to scoring are gated by the new options. This is controlled behavioural verification, not a promise of identical wall-time search trees.

The profile remains the Round 5 base: concurrency 1, ordered full deck, pairs together, kept processes. These changes do not alter warm-up, timeout policy or the known GUI/runner repetition boundary.

## 4. Recount and measured process ages

The final records head, Book Lab e2c1655, supplies the remaining E2 arms and all three full-deck concurrency-1 runs. There is no longer a missing-PDN gap for the requested records.

All new games replay legally and their PDN results reconcile to CSV by numeric pair/leg. The complete tagged E2/match-order corpus is now 52 PDNs, 6,720 games and 679,992 plies: Cake 4,776 games, KingsRow 1,320, GuiNN 624. Since round 4 this adds 3,360 games: 72 wins, 3,287 draws, one loss.

The whole corpus contains 2,928 games with separate EngineGamesA/B and 3,792 with the legacy shared EngineGames. Legacy rows retain the shared observation in age; age_A/age_B explicitly say unavailable and age_form says shared. No separate-counter discrepancy or preceding-history gap was found. All 73 new decisive games have separate counters, equal in every case.

### Remaining E2 arms

The initial 14 new E2 PDNs recount as follows:

| Run | Games | Regicide W/D/L |
|---|---:|---|
| e2c-fresh-3 h1+h2 | 312 | 0/312/0 |
| e2c-fresh-4 h1+h2 | 312 | 0/312/0 |
| e2c-kept-4 h1+h2 | 312 | 0/312/0 |
| e2c-kept-5 h1 | 156 | 0/156/0 |
| e2d-kr134-fresh | 24 | 0/24/0 |
| e2d-kr134-kept | 24 | 2/22/0 |
| e2d-kr134-natural | 48 | 9/39/0 |
| e2d-kr16-fresh | 24 | 24/0/0 |
| e2d-kr16-kept | 24 | 9/15/0 |
| e2d-kr16-natural | 48 | 5/43/0 |
| e2d-kr43-natural | 48 | 1/47/0 |
| **Subtotal** | **1,332** | **50/1,282/0** |

The last records push additionally supplies e2c-kept-5 h2, e2c-kept-6 h1+h2, e2c-fresh-5 h1+h2 and e2c-fresh-6 h1+h2: seven half-decks, 1,092 games, all drawn. Thus all six fresh and six kept deck repeats are now complete; each arm has 1,872 drawn games. Wrapper-died duplicate copies are not independent repeats.

All 50 new decisive E2 games are Regicide White wins. The exact per-game rows and both actual counters are in [new-decisive-games.tsv](tables/new-decisive-games.tsv), alongside the 23 new long-run decisive games.

| E2 arm | Wins by measured A/B age |
|---|---|
| #16 fresh | 0/0: 24 |
| #16 kept, same-colour games | 0/0: 8; 1/1: 1 |
| #16 natural pairs | 1/1: 2; 3/3: 3 |
| #43 natural pairs | 3/3: 1 |
| #134 kept, same-colour games | 1/1: 2 |
| #134 natural pairs | 1/1: 3; 3/3: 3; 5/5: 1; 7/7: 2 |

Workers with shorter games take additional pairs; observed ages 7 or 9 must not be reduced to an assumed maximum of 5.

For the five isolated natural-pair experiments, each with 24 White games:

| Opening/opponent | White W/D/L | Winning-game ages | Move at known ply |
|---|---|---|---|
| #16 KingsRow | 5/19/0 | age 1: 2; age 3: 3 | four wins play 9-14 at ply 41; one plays 32-28 |
| #43 KingsRow | 1/23/0 | age 3: 1 | winning game plays 8-12 at ply 23; the other 23 play 8-11 and draw |
| #71 KingsRow | 6/18/0 | age 1: 6 | all six play 10-14 at ply 25 |
| #134 KingsRow | 9/15/0 | age 1: 3; 3: 3; 5: 1; 7: 2 | all nine play 20-24 at ply 37; the other 15 play 11-16 and draw |
| #163 Cake | 10/14/0 | age 3: 8; age 7: 2 | all 24 play 4-8 at ply 15, wins and draws alike |

All 120 Black games in these natural arms draw. The preceding real game of every White game is the same opening's other colour in the same processes. Kept single-colour arms instead have a same-colour predecessor.

[known-points.tsv](tables/known-points.tsv) retains exact FENs, moves, phases, results and ages. A named ply alone does not guarantee an identical position; for example the natural #71 games reach two FENs there. The first-variation analysis below requires an identical complete prefix. These are associations, not a randomized causal estimate.

### The three concurrency-1 runs

| Run | Games | Regicide W/D/L | Worker | A/B ages |
|---|---:|---|---|---|
| mo1-cake | 312 | 0/312/0 | 0 throughout | 0..311, equal throughout |
| mo1-kingsrow | 312 | 1/310/1 | 0 throughout | 0..311, equal throughout |
| mo1-guinn | 312 | 21/291/0 | 0 throughout | 0..311, equal throughout |
| **Total** | **936** | **22/913/1** | | |

Every game follows the full 156-ballot deck in order, primary Black first and then White. Both tagged process ages equal game_number-1 in all 936 games, including after the KingsRow loss. There is no age reset or missing previous process game.

These are the committed 1c6e997 runs using the earlier on-board result policy, not the new --cb-claims/no-loss profile. The records must not be represented as measurements of that later profile.

The KingsRow loss is **ACF 6, Regicide Black, round 5.1, game 9, EngineGamesA=8 and EngineGamesB=8**, ending with Black having no legal moves. It is a played-board loss, not a recorded concession or time forfeit. The KingsRow win is ACF 43, Regicide White, round 39.2, game 78, both ages 77.

All 23 decisive long-run games:

| Opponent | ACF | Our colour | Game | EngineGamesA | EngineGamesB | Result |
|---|---:|---|---:|---:|---:|---|
| KingsRow | 6 | B | 9 | 8 | 8 | L |
| KingsRow | 43 | W | 78 | 77 | 77 | W |
| GuiNN | 5 | W | 8 | 7 | 7 | W |
| GuiNN | 6 | W | 10 | 9 | 9 | W |
| GuiNN | 9 | W | 16 | 15 | 15 | W |
| GuiNN | 16 | W | 28 | 27 | 27 | W |
| GuiNN | 22 | B | 39 | 38 | 38 | W |
| GuiNN | 25 | W | 46 | 45 | 45 | W |
| GuiNN | 28 | B | 49 | 48 | 48 | W |
| GuiNN | 34 | W | 62 | 61 | 61 | W |
| GuiNN | 43 | W | 78 | 77 | 77 | W |
| GuiNN | 47 | W | 82 | 81 | 81 | W |
| GuiNN | 86 | W | 154 | 153 | 153 | W |
| GuiNN | 91 | W | 162 | 161 | 161 | W |
| GuiNN | 92 | B | 163 | 162 | 162 | W |
| GuiNN | 96 | W | 170 | 169 | 169 | W |
| GuiNN | 101 | W | 180 | 179 | 179 | W |
| GuiNN | 108 | B | 193 | 192 | 192 | W |
| GuiNN | 112 | B | 199 | 198 | 198 | W |
| GuiNN | 129 | B | 231 | 230 | 230 | W |
| GuiNN | 146 | W | 262 | 261 | 261 | W |
| GuiNN | 153 | B | 275 | 274 | 274 | W |
| GuiNN | 174 | W | 312 | 311 | 311 | W |

Each decisive termination is that the losing colour has no legal moves. Cake has no decisive game. [c1-decisive-games.tsv](tables/c1-decisive-games.tsv) includes exact round and termination. [c1-run-recount.tsv](tables/c1-run-recount.tsv) gives the independently recounted totals.

### Predicted ages versus actual records

For each of all three opponents, both colours at every requested opening match the prediction:

| ACF | Black game / predicted A/B age | White game / predicted A/B age | Actual |
|---:|---|---|---|
| 16 | 27 / 26 | 28 / 27 | both counters match, all three opponents |
| 43 | 77 / 76 | 78 / 77 | both match, all three |
| 71 | 125 / 124 | 126 / 125 | both match, all three |
| 134 | 241 / 240 | 242 / 241 | both match, all three |
| 163 | 293 / 292 | 294 / 293 | both match, all three |

[c1-age-check.tsv](tables/c1-age-check.tsv) contains all 30 opening/colour/opponent checks, not just a sample.

At the known White-game decision plies:

| Opponent/opening | A/B age | Known ply | Opponent move | Result |
|---|---|---:|---|---|
| KingsRow #16 | 27/27 | 41 | 32-28 | D |
| KingsRow #43 | 77/77 | 23 | 8-12 | W |
| KingsRow #71 | 125/125 | 25 | 5-9 | D |
| KingsRow #134 | 241/241 | 37 | 11-16 | D |
| Cake #163 | 293/293 | 15 | 4-8 | D |

[c1-known-points.tsv](tables/c1-known-points.tsv) gives each FEN. In particular #43's 8-12 also occurs at age 77 in the long match; it is not confined to the isolated natural arm's age-3 win. The #71 age-1 error and #163 isolated natural wins are not reproduced at their long-match ages.

### Extended process-state tables

The round-4 prefix-tree method is unchanged: group by opponent/opening/our colour/start FEN; use referee-canonical moves and complete identical prefixes; no transposition merging; exclude imposed first three plies. Each observation is a game's first varying opponent turn.

| Opponent | First-variation observations | Prefix positions | Positions varying across age buckets | Observations at those positions |
|---|---:|---:|---:|---:|
| Cake | 4,041 | 263 | 263 | 4,041 |
| KingsRow | 1,317 | 311 | 37 | 495 |
| GuiNN | 544 | 272 | 27 | 54 |

GuiNN now has two tagged full-deck runs per opening/colour, so first-variation comparisons are possible. Its zero-observation limitation from round 4 is resolved. Coarse age 2+ still merges most old and long-match games; exact tagged ages remain in the per-game tables.

Evidence:

- [new-run-recount.tsv](tables/new-run-recount.tsv): all 24 newly supplied runs since round 4.
- [record-sources.tsv](tables/record-sources.tsv), [source-integrity.tsv](tables/source-integrity.tsv): all 52 PDNs, counts, tag type and decompressed hash.
- [new-decisive-games.tsv](tables/new-decisive-games.tsv): all 73 new decisive games, both measured counters, worker and prior game.
- Process history: [part 1](tables/process-games-1.tsv), [part 2](tables/process-games-2.tsv).
- First varying move per game: [part 1](tables/first-varying-move-games-1.tsv), [part 2](tables/first-varying-move-games-2.tsv).
- [first-varying-move-joint.tsv](tables/first-varying-move-joint.tsv): conditional counts/results by prefix, choice, age bucket, previous-game category and phase.

The two largest tables are split to keep every report file below 1 MB. Each part retains its header. Expanded corpora can expose an earlier varying move, so local node IDs must not be joined directly to earlier report IDs. Build/phase, exact previous lines and timing remain confounded with process age; these records locate associations, not causes.

## Evidence and publication

[fixture-output.txt](evidence/fixture-output.txt) contains the latest API/runner counterexamples, score-route tests, before/after no-loss calls, and extracted CheckerBoard reference results, with each section's filename. The exact history commands are linked beside their cases.

A dash in a table means a field does not apply. Only this report, its text evidence and TSV tables are committed under reviews/a/round6. No master/feature code was changed.
