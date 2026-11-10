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
- Baseline (before the fix): 60 random positions (seed 9: GuiNN 35, KingsRow 24, Cake 1), `bl-r6`; running.

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
