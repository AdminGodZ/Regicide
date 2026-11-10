# booklab results (state 2026-11-10 06:40 CET; updated after every validation run)

Settings everywhere: acf156, 0.5 s/move (CheckerBoard rules), `-c 8`, hash 128, opponents with their own books and the
KingsRow 8-piece WLD db, Regicide with its 2–8-piece db, PDN comments on. Each 312-game run = two halves of 78 ballots
× 2 colours (`scripts/acf156_h1.txt`, `acf156_h2.txt`), one timing-lock hold per half. Raw `summary.txt`/`results.csv`
per run in `tests/`.

## WP1 census (`census/census.md`, `census/decisive.csv`, `census/instant_by_ballot.csv`)

| Pairing (A vs B) | Games | A: W / D / L | Elo of A [95% CI] |
|---|---|---|---|
| Cake vs KingsRow | 312 | 13 / 299 / 0 | +14.5 [+6.9, +22.1] |
| Cake vs GuiNN | 312 | 16 / 294 / 2 | +15.6 [+6.6, +24.6] |
| KingsRow vs GuiNN | 312 | 2 / 303 / 7 | −5.6 [−12.1, +0.9] |
| Cake vs Cake | 312 | 1 / 310 / 1 | 0 |
| KingsRow vs KingsRow | 312 | 2 / 310 / 0 | +2.2 [−0.8, +5.3] |
| GuiNN vs GuiNN | 312 | 3 / 304 / 5 | −2.2 [−6.6, +2.1] |
| Regicide master (6bb8c6c-deep2b DLL = fcf6acb) vs GuiNN | 312 | 1 / 309 / 2 | −1.1 [−4.9, +2.7] |
| (reference, same DLL, 2026-11-09) Regicide master vs Cake | 312 | 0 / 307 / 5 | −5.6 [−10.4, −0.8] |
| (reference, same DLL, 2026-11-09) Regicide master vs KingsRow | 312 | 0 / 312 / 0 | 0 |

Decisive games (ballot, winner as colour):
- **Cake beats KingsRow (13):** #23 B, #36 W, #28 B, #59 W, #86 Octopus W, #84 W, #99 W, #128 W, #139 B, #151 B, #154 W,
  #155 B, #166 W.
- **Cake beats GuiNN (16):** #9 Wilderness I W, #6 Inferno W, #16 Wilderness II W, #34 Twilight Zone W, #41 W, #40 W,
  #60 Black Hole W, #87 Tyne W, #92 W, #96 Skull-Cracker W, #112 B, #108 B, #120 B, #134 W, #141 W, #153 Black Widow B.
  **GuiNN beats Cake (2):** #163 B, #165 B.
- **KingsRow vs GuiNN:** GuiNN wins #104 B, #99 B, #105 Gemini II B, #139 W, #138 W, #142 W, #172 B; KingsRow wins #98 W,
  #169 W.
- Mirrors: Cake–Cake only #165 (Black wins with either process); KingsRow–KingsRow #9 Wilderness I (W), #134 (B);
  GuiNN–GuiNN #6 Inferno (W), #16 Wilderness II (B, both processes), #28 (B, both), #87 Tyne (W), #101 (W), #149 (B).
- Regicide vs GuiNN: GuiNN wins #59 (W) and #63 (W); Regicide wins #153 Black Widow as Black.

**Book depth (leading moves after the ballot that took < 0.05 s; in brackets the leading moves the engine's own
comment marks as book), mean / median over all census games:**

| Engine | As Black | As White |
|---|---|---|
| Cake | 7.4 / 7 (book 7.3 / 7) | 7.5 / 7 (7.4 / 6) |
| KingsRow | 8.9 / 8 (8.5 / 8) | 8.5 / 8 (8.1 / 8) |
| GuiNN | 7.5 / 5 (6.3 / 4) | 8.4 / 6 (7.5 / 5) |
| **Regicide master** | **1.4 / 1 (1.1 / 0)** | **1.3 / 0 (1.1 / 0)** |

Regicide leaves its book (or never had one) after about one move in half of all ballots; the three opponents play
7–9 book moves. Per ballot: `census/instant_by_ballot.csv`.

Determinism (first half of acf156 played twice, game by game, same ballot and colours):
- **Cake–Cake:** 54 of 156 games identical (35%); the others first differ at a median of 31 plies after the ballot
  (quartiles 21–44, min 10). Results identical (156 draws both times).
- **Regicide master–GuiNN:** 6 of 156 identical (4%); first difference at a median of 7 plies after the ballot
  (quartiles 2–15, min 0): GuiNN varies its book. Yet the results repeat exactly: the repeat lost the same two
  ballot-colours (#59 B, #63 B) and nothing else (0 / 154 / 2 both times).

From the 2026-11-09 targeted runs: Regicide vs Cake, same 100 games twice: 12% identical
games, the rest first differ at a median of 34 plies after the ballot (quartiles 24–43, min 11); the losing lines
repeat almost exactly (Black Widow: all 60 lost games share the first 20 plies after the ballot).

## WP5 repairs and targeted tests

| Round / DLL | Change | Test | Result |
|---|---|---|---|
| r1 `bl-r1` | Cake repairs: Black Widow W (26x19 + Cake defence, 146 positions), #7 B (3-7 at ply 22, 59), Gemini I B (7-10 at ply 8, 308), #79/#90 B (1-6 at ply 22, 47) | vs Cake, 10 per ballot and colour | **100 / 100 draws** (master: BW 59/60 lost, #7 39/60) |
| | | vs KingsRow, vs GuiNN, 4 per ballot and colour | 40 draws, 40 draws |
| r2 `bl-r2` | + GuiNN repairs: #59 B (3-7 at ply 4, 131 positions), #63 B (9-13 at ply 12, 64) | vs GuiNN 10 per colour; vs Cake, KingsRow 4 | 40 draws; 16 draws; 16 draws |
| r3 `bl-r3` | + 7 sweep forces (#5, #12, #34, #76, #132, #174, Gemini I ply 10) | 4 per ballot and colour vs each engine | Cake: **#12 B lost 4/4**; GuiNN: **#34 B lost 1/4**; KingsRow 56 draws |
| r4 `bl-r4` | r3 without the #12 sweep | validation 1 (run a only) | see below; #34 lost once more |
| **r5 `bl-r5`** | r4 without the #34 sweep, + Twilight Zone ply-44 force (WP5b) | validation 2 | running |

Evidence per decision is in `decisions.txt` (and `repairs/`). Rejected forces: `repairs/repairs_s3/rejected.txt`.

**Twilight Zone (#34, B) vs Cake, validation 1:** with the #34 sweep force (7x16 instead of Regicide's 8x15), Regicide
followed Cake's defence for 18 moves and lost an 11–12-piece ending: deep analysis (bookgen, 100M nodes per move, 8-piece
db, 2 GB hash) gives 11-15 = 0 and Regicide's 2-7 (0.92 s, own score −6) = −94 at ply 44; at plies 46 and 48 Regicide's
moves were already best (−96, −94). So the loss came from the forced 7x16 line plus one endgame mistake. Both fixes are
in r5 (sweep dropped; 11-15 forced at ply 44, harmless if not reached).

## WP6 validation

| Round | DLL | Opponent | Run a | Run b |
|---|---|---|---|---|
| 1 | bl-r4 | Cake | 0 / 311 / 1 (#34 B, the rejected sweep) | stopped (r4 superseded) |
| 1 | bl-r4 | KingsRow | 1 / 311 / 0 (win #147 B) | stopped |
| 1 | bl-r4 | GuiNN | 3 / 309 / 0 (wins #9 W, #111 B, #141 W) | stopped |
| 2 | bl-r5 | Cake | **0 / 312 / 0** | queued |
| 2 | bl-r5 | KingsRow | **0 / 312 / 0** | queued |
| 2 | bl-r5 | GuiNN | 0 / 311 / 1 (#96 Skull-Cracker B, new) | queued |

**Skull-Cracker (#96, B) vs GuiNN, validation 2:** Regicide followed Cake's and KingsRow's moves through ply 20 and
left them at ply 22 with 7-11 (Cake there: 12-16, 21 draws in 21). The repair keyed on GuiNN's own moves is weak here
(GuiNN's 1-5 at ply 14 has a loss), so the repair follows Cake's defence (`theory.py repair --opp cake`): 12-16 + Cake's
defence tree, 124 positions (`repairs/repairs_v2`). r6 = r5 + this repair (`bl-r6`); per the lead's instruction only
this ballot is re-tested (both colours, 4 games vs GuiNN) and validation 2 continues with bl-r5.

## WP8: missed database wins (`wp8/dbscan.md`, regression set `missed_wins.txt`)

Every games.pdn under `~/regicide-match/results` (all Regicide versions and all agents' runs: baselines, ba-*, census,
tests, validation, pt-*, tr-*), Regicide vs Cake/KingsRow/GuiNN plus the games among the three; 2,964 duplicate games
removed. Each game replayed (`booklab_dbscan`, `tools/booklab/dbscan.cpp`) and every position with ≤ 8 pieces probed
in Regicide's 2–8-piece WLD database.

| Games | Scanned | Reached a database win | Winning side won | Not won |
|---|---|---|---|---|
| Regicide vs Cake/KingsRow/GuiNN | 21,013 | 1,925 | 1,554 | 371 |
| Opponent vs opponent | 2,564 | 73 | 63 | 10 |

**Conversion by the side that had the database win: Regicide 37 of 399; Cake 1,011 of 1,017; KingsRow 223 of 231;
GuiNN 346 of 351.**

- **Table 1, Regicide had a database win and did not win: 362 games.** In every one the first won position had 7 or 8
  pieces (346 at 8, 16 at 7; none at ≤ 6, where the database has distance information). 253 ended still
  database-won (the rules ran out: 312 of all 362 by threefold repetition, 50 by the 40-move rule); in 109 Regicide
  later moved into a non-won position. It happens in every version, including the current book:
  validation 2 (bl-r5) 15 of 15 won positions not converted, validation 1 (bl-r4) 28 of 32, census Regicide–GuiNN
  (master) 30 of 31, deep2b full runs 9 of 9, ponder runs 26 of 27.
- Cross-check against the opponents' own engines: in the census Regicide–GuiNN missed wins GuiNN's own eval at the
  first won position is about −440 to −790 from its side, and Regicide's search reports 19999 (database win) on every
  move until the repetition. So the positions are won by both programs' accounts; the database is not the problem.
- **Table 2, an opponent had a database win against Regicide and did not win: 9 games**, all in old baselines
  (a44591e, ae210fb, b71dfa9; Cake 4, GuiNN 5), all 8-piece positions, 6 by repetition / rule with the position still
  won. The opponents convert 99% of their database wins.
- **Table 3, opponent vs opponent: 10 games** (see `wp8/dbscan.md`), e.g. KingsRow failed to convert four won endings
  against GuiNN in the census by the 40-move rule (three with 6 pieces). Here KingsRow runs with the WLD-only
  8-piece database (`kingsrow_wld8`), which has the same lack of distance information.
- Consequence: against GuiNN alone master had 30 won 7–8-piece endings in one 312-game census run and won one. The
  conversion fix (t3/wld-conversion) is worth far more than any book change; `missed_wins.txt` (362 FENs, first
  won position per game, Regicide to move or just moved) is its regression set.

**WP8 correction check (2026-11-10 09:30, review point: results other than 1-0/0-1 were counted as draws).**
`dbscan.py` now skips games without a final result ("*" or none) and reports how many it skipped; it also leaves out
the WP9 test runs (`bl-wp9*`, they start from won positions) and can restrict the corpus by time (`--until`).
- Result: **0 games without a final result** in the corpus (the match runner never writes aborted games to the PDN), so
  the bug had no effect. Re-scanning the same corpus as of 07:50 reproduces exactly the same table and the same 362
  missed wins (**0 of the 362 drop out**). (The 362 rows hold 349 distinct FENs: some first-won positions recur.)
- On the corpus as of 09:30 (more games since, WP9 tests excluded): Regicide converted 37 of 410 database wins
  (373 missed: the 362 plus 11 from this morning's validation-2 runs with bl-r5, 6 vs KingsRow and 5 vs GuiNN); Cake
  1,011 of 1,017; KingsRow 226 of 248; GuiNN 347 of 352. Opponent vs opponent: 90 reached, 66 converted (the new games
  are other agents' runs). The scan now also records the longest no-progress run per game.

## MERGE CANDIDATE: round 10 (`decisions.txt`, `book_booklab.bin`)

**book_r10.bin, sha256 `cb4e2bb2aea4d6de6bc789513baa8b49ae70b06d5d37b064adbf1a6018197a1e`, 987 decisions** = the
validated r8 (991 decisions, sha256 `63e7fd19481b470abd0e7cca5608d41c9b6927d8457daf24ea074cabf783e022`) minus only:
- the 3 decisions proved unreachable (r8 lines 633 #59, 846 #62, 1160 #10; exhaustive search with the engine's book
  eligibility, also at 20M nodes);
- #34 line 853 (its only line runs through 7x16 at ply 4, which the book no longer plays since r5).
Record: `removed_decisions.txt` (the withdrawn round-9 list of 173 is kept as `removed_decisions_r9_withdrawn.txt`).

Checks:
- Reachability under book_r10 (engine's eligibility, witnesses + exhaustive search): 822 proved reachable, **0
  unreachable**, 165 unknown (kept, per the lead's rule). The 20M-node search on the 170 earlier unknowns proved 8
  reachable and none unreachable beyond the 3.
- **Same play as r8:** every position with Regicide to move in the 3,046 games played with the r6/r7/r8 books (WP9
  steps 3–4, the re-tests, the final validation so far) probed in both books: **110,157 positions, 0 differ**; none of
  the 4 removed positions occurred. (`book_equiv_r10.md`; re-run when the final validation ends.)
- r8 reproduces byte-identically from the repair files with the current assemble.sh.

## Pre-merge review fixes (branch merged with master 456e9b2: ctest 27/27, bench 7,677,736)

- **B1** `movetext.hpp`: two squares one jump apart name that single jump when it is legal (the exact reading wins,
  as in master's parse_move and referee since PR #38); only otherwise is the text an abbreviation, rejected when
  ambiguous. `22x29` in `W:WK22:B9,10,17,18,25` is now accepted by booklab_apply and booklab_dbscan. Books r8 and w1
  rebuild byte-identical.
- **B2** `theory.py`: a game without a recognised final result is skipped, and a game with any replay error (E) or
  ambiguity notice (A) is dropped entirely before anything is counted. Rebuilt table: 128,189 games, **0 skipped,
  0 dropped** (0 E, 0 A lines), so every count equals what the old code gives on the same corpus: **no decision's
  evidence changes**.
- **B3** `assemble.sh`: identity of a row is move **and** action (force/avoid); a different action is a conflict.
- README: the job scripts and status logs are marked as archived machine-specific jobs.

## FINAL VALIDATION: bl-final1 = master 2efe11a + round-8 book

(a) The same 100 missed-win positions as bl-conv3: **converted 100 of 100** (Cake 3/3, GuiNN 59/59, KingsRow 38/38;
`wp9/report-wp9final1.md`); plies to the win median 112, quartiles 93–130, max 180; highest no-progress count median 26,
max 53 (2 games ≥ 40, none ≥ 60); no game left the won region.

(b) Two full acf156 runs against each opponent (`scripts/chain_final.sh`; runs `tests/f1-<opp>-<a|b>-<half>`):

| Opponent | Run a | Run b | master fcf6acb (before) |
|---|---|---|---|
| Cake | **0 / 312 / 0** | running | 0 / 307 / 5 |
| KingsRow | **12 / 300 / 0** | queued | 0 / 312 / 0 |
| GuiNN | h1 8 / 148 / 0, h2 running | queued | 1 / 309 / 2 |

Repairs during the final validation: none so far.

## Book hand-over checks (round 9)

- **Reachability check in the build** (`tools/booklab/reach.cpp` → `booklab_reach`, `reach_check.py`, run by
  `scripts/make_book.sh`): for each decision, games of the theory table that reach its position from an ACF ballot are
  replayed against the book being built (wherever our side is to move and the book has a move, the game must play
  it); one consistent game proves the decision reachable. The rest go to an exhaustive search from the decision's
  ballot (our side: the book's move where it has one, else any legal move; opponent: any legal move; pruned by piece
  counts and men only moving forward; node limit), which proves unreachability or reports "unknown".
- **r8 (991 decisions):** 818 reachable (814 by a witness game), **3 proved unreachable** (exactly the reviewer's
  three: r8 lines 633 #59, 846 #62, 1160 #10), 170 unknown. All 170 unknown, and #34 line 853, have every known line
  to them cut by one of our own forced moves earlier in the same defence tree (e.g. 44 decisions of the #59 tree lie
  under 11-16 at ply 6, where the book forces 14-18): minority branches of defence trees, never reached in any recorded
  game under this book. Line 853 (#34, ply 44): its only line went through 7x16 at ply 4, which the book no longer
  plays (8x15 since r5); dropped.
- **Round 9 = r8 minus those 173** (`removed_decisions.txt`: FEN | move | reason | original evidence; assemble.sh leaves
  these positions out): **818 decisions, all 818 proved reachable**; `book_booklab.bin` = book_r9. The final
  validation (bl-final1) runs with the r8 book; r9 differs from it only in the 173 positions that no recorded game
  reaches under this book.
- **Superseded (lead's caution):** "unknown" is not "unreachable". `book_equiv.py` replayed every game played with
  the r6/r7/r8 books (2,448 games, 93,624 positions with Regicide to move): one of the 173 removed positions occurred
  (r8 line 943, Skull-Cracker ply 52, 15-18, once), and r8 and r9 differ in exactly that position. So r9 is **not** the
  merge candidate; the candidate is r8 minus only proved-unreachable decisions (being rebuilt; see below).
- **reach.cpp now uses the engine's book eligibility** (review point): a book move is treated as forced only where
  Engine::go would consult the book (`reversible_plies() == 0`, i.e. not inside a run of king moves, and the root not
  decided by the endgame database), with the database passed to the tool. On r8 **3 verdicts change** (lines 963, 979,
  990: unknown → reachable); 821 reachable, the same 3 proved unreachable, 167 unknown. A deeper exhaustive search
  (20M nodes per decision) on the 170 is running.
- Referee check (PR #38): no game in any result directory on this machine ended as "illegal move"; the only
  terminations are no legal moves, no progress and repetition.
- **assemble.sh** now fails on a conflict (two rounds give different moves for the same position: both rows printed),
  and still drops same-move duplicates (39) silently but counts them; r9 assembles with 0 conflicts.

## Database-lost positions entered by Regicide in step 4: answer (b), an artefact (`wp9/entered_losses.md`)

Question: "KingsRow had a database win in 16 games against bl-conv3 and converted one" — real (a) or artefact (b)?
**It is (b).** All 15 unconverted cases (and 14 against bl-m11e) are the **final position** of a game already drawn by
rule (threefold repetition or the 40-move rule): the move that completed the draw left the opponent a capture, the
game ended there, and that position was never played. The earlier count also had the dedup problem (identical games
of the two builds merged).

Method: `dbscan.py --quiet --no-dedup` over the step-4 runs (a database win counts only when the side to move has no
capture pending; identical games of both builds kept; first won position per side), and — the actual fix — a game's
final position never counts as a reached win (`booklab_dbscan`: only positions from which a move was played).

| Build | Opponent | Games | Opponent had a db win in play | it converted | Regicide had a db win in play | it converted |
|---|---|---|---|---|---|---|
| bl-conv3 | Cake | 312 | 1 | 1 | 0 | 0 |
| bl-conv3 | KingsRow | 312 | 1 | 1 | 11 | **11** |
| bl-conv3 | GuiNN | 312 | 0 | 0 | 20 | **20** |
| bl-m11e (no fix) | Cake | 312 | 0 | 0 | 0 | 0 |
| bl-m11e | KingsRow | 312 | 0 | 0 | 13 | **0** |
| bl-m11e | GuiNN | 156 (h2 running) | 0 | 0 | 12 | **3** |

The two games in which an opponent had a database win against bl-conv3 are the two step-4 losses; Regicide saw both
long before (its own score was already ≤ −100 with 16 and 20 pieces), so they are opening losses, not walked-in
endgames, and both are repaired (r7 pins #7, r8 repairs #10):

| Run | Ballot | Regicide as | First lost ply (pieces) | Regicide's score on its move before | First Regicide score ≤ −100 | bl-m11e up to that ply |
|---|---|---|---|---|---|---|
| s4-conv3-cake-h1 6.1 | ACF #7 | B | 93 (8) | −19999 | ply 26, 16 pieces | deviated at ply 20 (Regicide: m11e 16-19, conv3 16-20) |
| s4-conv3-kingsrow-h1 9.1 | ACF #10 | B | 80 (7) | −19999 | ply 18, 20 pieces | deviated at ply 3 (KingsRow played 24-19 / 25-22) |

`entered_losses.txt` holds these two (first lost position and four plies earlier) for completeness; (a) is not
confirmed, so there is no larger regression set.

**The same artefact in WP8:** 10 of the 362 "missed wins" were final positions of games drawn by the 40-move rule (all
old baselines vs GuiNN; list in `wp8/dropped_final_position.txt`). **Corrected WP8: Regicide had a database win in
play in 389 games and converted 37; 352 missed** (`missed_wins.txt` now holds the 352). Opponent vs opponent: 66 in
play, 62 converted. WP9 on the corrected set: bl-conv1 352 / 352 (it also won the 10 dropped ones), bl-conv3 98 / 98
of its sample (2 of its 100 were dropped ones, both won), the old engine 1 of 55 (5 of its 60 were dropped ones).

## WP9 step 4: full runs with the conversion fix (`wp9/s4_scan.md` tables)

Full acf156 runs (312 games each) with **bl-conv3** (engine 2156359 + round-6 book) and **bl-m11e** (its master base
11e19b4 + the same book, no conversion fix), interleaved by half (`scripts/chain_wp9.sh`, `chain_final.sh`). (bl-conv2
was built, ctest 17/17, but no run was started with it, per the switch to bl-conv3.) "In play": a database win in a
position from which the game went on (`s4_report.sh`, no dedup).

| Opponent | bl-conv3 W / D / L | Elo | bl-m11e (no fix) W / D / L | Regicide's in-play db wins converted: conv3 / m11e | Opponent's in-play db wins vs conv3 (converted) |
|---|---|---|---|---|---|
| Cake | 0 / 311 / 1 (#7 B) | −1.1 | 0 / 312 / 0 | 0 / 0 – 0 / 0 | 1 (1) |
| KingsRow | **11 / 300 / 1** (#10 B) | +11.1 | 0 / 312 / 0 | **11 / 11** – 0 / 13 | 1 (1) |
| GuiNN | **20 / 292 / 0** | +22.3 | 6 / 306 / 0 | **20 / 20** – 5 / 21 | 0 |

Both bl-conv3 losses are opening losses, repaired since (#7 by the r7 path pins, #10 by the r8 repair; re-tests 4/4
draws each). Elo is computed from the total score of the 312 games.

- s4-conv3-cake-h1: 0 / 155 / 1. The loss: **ACF #7 as Black**. With the new engine Regicide's own search played
  16-20 at ply 20 instead of 16-19 (the move Cake plays there, 88 draws in 88, and the move the old engine played), one
  ply before the r1 repair's forced line starts. Engine changes can shift unforced moves on the path to a repair.
- **Round 7 (`decisions.txt`, 920 force lines):** r6 + the pinned path of every repair (`repair_all.py --pin-only`:
  each move before the deviation that the opponent also played there without losing; 42 lines: #7 9, #79 9, #90 9,
  #96 9, #63 4, Gemini I 2). DLL `bl-r7c` = engine 2156359 + round-7 book (bench 7,677,736). Per the instruction, only
  the affected ballot is re-tested (#7, both colours, 4 games vs Cake); step 4 continues with bl-conv3.

## WP9 step 3: the 362 missed wins against the original opponents (`wp9/report-wp9conv1.md`)

Every position of `missed_wins.txt` played once from that position, Regicide (engine A) with the side that has the
database win, against the opponent of the original game with its usual 8-piece database; 0.5 s/move, threefold
repetition and the 40-move rule, timing lock per deck of ≤ 40 positions (`scripts/wp9_run.sh`).

| DLL | Positions | Converted | Cake | GuiNN | KingsRow | 7 pieces | 8 pieces | Left the won region |
|---|---|---|---|---|---|---|---|---|
| bl-r6 (before: engine fcf6acb, round-6 book) | random 60 | **1 (1.7%)** | 0/1 | 0/35 | 1/24 | 0/3 | 1/57 | 26 games |
| **bl-conv1** (engine 714502c + round-6 book) | all 362 | **362 (100%)** | 6/6 | 198/198 | 158/158 | 16/16 | 346/346 | **0** |
| **bl-conv3** (2156359 + round-6 book) | random 100 of the 362 (seed 13; bl-conv1 failed none) | **100 (100%)** | 3/3 | 59/59 | 38/38 | 8/8 | 92/92 | **0** |

- bl-conv3: plies to the win median 114, quartiles 93–129, max 235; no-progress maximum median 27, max 41 (2 games
  ≥ 40, none ≥ 60). Report: `wp9/report-wp9conv3.md`.

- bl-conv1, plies to the win: median 109, quartiles 95–129, max 210.
- Highest no-progress count (plies without a capture or man move; the 40-move rule draws at 79) in the converted
  games: median 27, max 63; 12 games reached ≥ 40 plies, 2 ≥ 60, none ≥ 70.
- No failures, so there is no failure list; the per-game table (plies, no-progress maximum) is in the report.

## WP9 step 2: conversion_test with the real 2–8-piece database (bl-conv1 tree, engine 714502c)

`conversion_test --dir ~/regicide-data/egdb --pieces P --kind both --sample 200 --cap 32 --nodes 100000
--oracle-nodes 1000000 --role R`, run as 4 processes (P = 7, 8; R = win, lose; `scripts/ct_run.sh`, logs `wp9/ct/`).

| Pieces | Engine converts (vs 1M-node oracle) | Mean plies engine / reference | Engine defends (oracle must win) | Mean plies engine / reference |
|---|---|---|---|---|
| 7 | **198 / 200** (2 draws by the no-progress rule) | 62.0 / 53.9 | oracle won 200 / 200 | 51.5 / 53.9 |
| 8 | **200 / 200** | 50.0 / 45.0 | oracle won 200 / 200 | 45.1 / 45.0 |

The two 7-piece failures (no-progress draws after 142 and 177 plies; MTC-optimal 67 and 40):
`W:W8,18,K20,28:B6,K17,K25`, `B:W15,24,K28,K31:B13,18,K30`.

**Speed in won 7–8-piece roots:** the CLI from the same tree, `go nodes 3000000` with the 8-piece database: 4.3M nodes/s
(score +10107, the conversion score) and 5.2M nodes/s in two won/lost 8-piece positions, against 2.9M in the start
position, all measured while matches and the four test processes ran. So the database probe at every node does not
slow the search down here; it is faster than in the middlegame.

## WP9: conversion test harness

- **tools/match change (20 lines, `scripts/patch_match_colour.py`):** an openings-file line may carry a third field,
  `FEN | moves | A=B` or `A=W`, which fixes engine A's colour for that opening; the game with colours reversed is not
  scheduled ("Games: N of 2N scheduled"). Lines without the field behave as before; referee test passes.
- `tools/booklab/wp9.py decks` writes one deck per opponent (≤ 40 positions per deck, so each lock hold stays under
  about 20 min) from WP8's table 1: every first database-won position, Regicide = A with the winning side, against the
  opponent of the original game (GuiNN 198, KingsRow 158, Cake 6 positions). `wp9.py report` replays the games with
  `booklab_dbscan`: converted / not converted per opponent and piece count, plies to the win, every non-converted
  position with its ending and last 20 moves, and every game where Regicide moved from a won position into a
  non-won one. Runner: `scripts/wp9_run.sh DLL TAG POSDIR` (0.5 s/move, baseline settings, timing lock per deck).
- **Baseline (before the fix), `bl-r6` (engine fcf6acb + round-6 book), 60 random positions (seed 9), 0.5 s/move:
  converted 1 of 60** (Cake 0/1, GuiNN 0/35, KingsRow 1/24; 7 pieces 0/3, 8 pieces 1/57). In 26 of the 60 games
  Regicide moved from a database-won position into a non-won one; the rest ended still won by repetition or the
  40-move rule. Report: `wp9/report-wp9base.md`.

## Review fixes (2026-11-10, after 8535bca)

1. **Capture identity.** `booklab_replay`, `booklab_show`, `booklab_apply` reports now write captures with their full
   jump path (`tools/booklab/movetext.hpp`); `booklab_apply` rejects a decision whose move text matches more than one
   legal move (e.g. `2x18` in `B:W6,7,14,15,32:B2`, where 2x9x18 and 2x11x18 are both legal; both full paths are
   accepted and forced correctly). The replay flags game-record tokens that match several routes. **Effect:** the
   rebuilt theory table (109,727 games, 11.85M plies) has 0 ambiguous tokens in game records and exactly one position
   where two routes with the same start and end were both played (Regicide self-play, 18x11x2 / 18x9x2; in no
   decision). Rebuilding book_r5 and book_w1 with the strict bookapply gives byte-identical books: no decision was
   affected.
2. **census.py pairs.** Only complete pairs (both games) enter the pentanomial Elo; unpaired games are counted and
   printed. **Effect:** none; every census pair was complete, the tables are unchanged.
4. **Second round (after fc5018d).** (a) Move identity in `movetext.hpp` is now start square, end square and the set
   of captured pieces, computed from the written path in PDN geometry: both 9x18x11x2x9 and 9x2x11x18x9 in
   `B:W6,7,14,15,32:BK9` are accepted as the same move (before: "matches 0 legal moves"); `2x18` in
   `B:W6,7,14,15,32:B2` is still rejected as ambiguous; books r6 and w1 rebuild byte-identical. (b) `census.py` prints
   "n/a (no complete pair)" instead of +0.0 when no pair is complete.
5. **Third round (scanner and runner).** `booklab_dbscan` rejects a game whose move does not parse to exactly one
   legal move (row `GID ERR ply token`) instead of scanning its prefix; `dbscan.py` checks every worker's exit status
   and that rows == games, deduplicates only after the result check, and lists duplicate records with the same moves
   but another result or termination; `movetext.hpp` rejects a path that jumps the same piece twice (9x18x9);
   `bl_run.sh` treats a run as done when its summary has the `A:` line (fixed-colour decks print no pentanomial Elo)
   and re-checks after taking the lock. **Effect:** on the 07:50 corpus 0 games rejected, 0 conflicts; WP8 numbers
   unchanged (Regicide 37 / 399, 362 missed); books r8 and w1 rebuild byte-identical.
3. **Regicide book marks.** `is_book()` now matches `depth\s+0/0\s+score\s+-?\d+\s+nodes\s+0\s`. The PDNs written by
   the match runner on this machine contain single spaces (0 double-space occurrences), so the old pattern already
   matched here. **Effect:** the regenerated book-depth column is identical.

## Regicide wins seen (for win mining)

| Run | Ballot | Regicide as | Opponent | Line after the ballot (first 30 plies) |
|---|---|---|---|---|
| census c-reg-guinn-h2 (master) | #153 Black Widow | B | GuiNN | 26x19 8-11 27-23 4-8 22-18 11-16 24-20 8-11 25-22 10-15 19x10 6x15 29-25 16-19 23x16 12x19 21-17 9-14 17x10 7x14x23 31-27 1-6 27x18 5-9 20-16 11x20 18x11 19-23 22-17 6-10 |
| v1-guinn-a-h1 (r4) | #9 Wilderness I | W | GuiNN | 18-14 10x17 21x14 6-9 25-21 9x18 23x14 8-11 29-25 11-15 24-19 15x24 27x20x11 7x16 25-22 4-8 22-18 8-11 26-23 13-17 31-26 3-7 30-25 16-19 23x16 12x19 32-27 19-23 26x19 17-22 |
| v1-guinn-a-h2 (r4) | #111 | B | GuiNN | 25-21 9-13 23-18 4-8 24-20 12-16 27-24 8-12 30-25 6-9 17-14 10x17 21x14 1-6 25-21 13-17 22x13 15x22 26x17 9x18 29-25 18-23 25-22 7-10 22-18 10-15 31-27 15x22 27x18 22-26 |
| v1-guinn-a-h2 (r4) | #141 | W | GuiNN | 17-14 10x17 21x14 6-9 25-21 9x18 23x14 8-11 29-25 16-20 24-19 11-16 26-23 2-6 25-22 6-10 22-18 10x17 21x14 7-10 14x7 3x10 18-15 4-8 15x6 1x10 23-18 16x23 31-26 13-17 |
| v1-kingsrow-a-h2 (r4) | #147 | B | KingsRow | 24-19 8-11 18-15 11x18 23x14 9x18 27-24 20x27 32x23x14 10x17 21x14 4-8 25-22 8-11 29-25 11-15 19x10 6x15 26-23 12-16 31-27 2-6 25-21 7-11 30-26 16-20 14-10 5-9 23-18 11-16 |

## Observations against the diagnosis

- Supports it: every Cake loss of master was an opening/early-middlegame deviation from the opponents' own
  non-losing moves, and copying their defence fixed all five ballot-colours at once (0 losses in 100 + 80 games).
  Master's book itself chose the losing Black Widow move: book_deep scores 27x18 at −35 and Cake's 26x19 at −47.
- Caution: theory evidence is only as good as the side it came from. Cake's 11-15 in #12 (6 draws, all as defender
  against Regicide's attack) lost 4 of 4 when Cake itself attacked; the #34 force (Cake 7x16, 30 games) led to a lost
  ending. Single-move forces taken from games against a weaker attacker need a test against the strong attacker.
- Not everything is opening: the one r4 loss came from a 12-piece ending 41 plies after the ballot.
- Cake is clearly the strongest opponent here: it beats KingsRow 13–0 and GuiNN 16–2 over 312 games, while
  KingsRow–GuiNN is roughly even.
- The machine was shared by three agents (truth data, ponder, booklab). The ponder session held the timing lock for
  full 312-game runs at `-c 4` (about 50 min each), so booklab runs often waited 30–50 min for the lock.
