# wld-conversion: converting database wins without distance information

Branch `t3/wld-conversion` (from master 6002841). The engine for the Windows test is commit **714502c**. Later
commits change only the test tool's comments, the scripts, the logs and this write-up, not the engine.

**Summary** (engine converting database wins; MTC withheld or capped; under the referee's rules):

| sample | master | this branch, 5k / 20k / 100k nodes per move |
|---|---|---|
| A: 3-piece wins, no MTC | 155/400 | 400 / 400 / 400 |
| A: 4-piece wins, no MTC | 218/400 | 398 / 400 / 400 |
| C: 5-piece wins, MTC up to 4 | 322/400 | 384 / 399 / 400 |
| C: 5-piece wins, men on both sides, MTC up to 4 | 333/400 | 395 / 398 / 400 |
| R6: real 6-piece wins, men on both sides, MTC up to 4 | 345/400 | 397 / 400 / 400 |
| R7: real 7-piece wins, men on both sides, real MTC 6, 100k-node defender | (not comparable) | 188 / 194 / 199 of 200 |
| Audit positions (MTC withheld) | 0/3 | 3/3 |

- Master's results do not depend on the node budget, because it does not search inside the won region.
- As defender, the engine now survives longer than master: in B, about 34–40 plies on average instead of 20 (the
  MTC-optimal reference is about 25).
- No game in any after run gave the win away: in a won root, only moves that keep the win are searched.

## Problem

The endgame database has win/draw/loss values for up to 8 pieces, but moves-to-conversion (MTC) only up to 6. When
the root is a database win with 7 or 8 pieces, master searches normally and cuts off every child with at most 8
pieces at once, all scored `SCORE_DB_WIN - ply`:
- all winning moves score the same;
- the search sees nothing inside the won region and has no notion of progress;
- the engine plays the first winning move that does not repeat a position.

So it shuffles until threefold repetition or the 40-move rule. Once every winning move repeats a position (scored as a
draw), it may also play a move that throws the win away.

The match scan (booklab `wp8/dbscan.md`): Regicide reached a database win in 399 games and won 37. All 362 misses
started at 7 or 8 pieces. On master, `conversion_test` reproduces the external audit exactly:
- `B:WK29:BK10,K16` drawn by repetition after 44 plies;
- `B:WK19:BK9,K28` win given away after 19 plies.

## Design as built

**Conversion mode** (`Engine::go`, `Worker::search`). It is on for a search when all of these hold:
- the root has at most `egdb_pieces` pieces;
- the database says Win or Loss for the side to move;
- `egdb_root_move` did not play the move. Either there is no MTC for this piece count, or the search is infinite
  analysis, where `egdb_root_move` is skipped already.

Every other search is unchanged: bench stays at **7,677,736**, the same as master 6002841.

In conversion mode, at a non-root node with at most `egdb_pieces` pieces:
- **Database draw**: return `SCORE_DRAW` at once, as before.
- **Win or loss, MTC available**: the node's piece count is within the engine's MTC limit, which is
  `min(egdb_pieces, max_mtc_pieces, set_egdb_mtc_pieces cap)`.
  - Return `±(SCORE_DB_WIN - ply - mtc)` at once, with MTC capped at 4000. The score therefore stays at or above
    15,872 > `SCORE_KNOWN_WIN`, and the ply adjustment in `score_to_tt`/`score_from_tt` stays valid.
  - The winner takes the fastest conversion and the loser the slowest.
  - Every MTC-covered win scores above every heuristic one, so the winner steers into the MTC region.
- **Win or loss without MTC**: no cutoff. The node is searched with the **conversion eval** as its static eval:
  `±(CONV_BASE + progress)`.
  - `CONV_BASE` is 10,000 and progress is clamped to ±4,000, so the value lies between 6,000 and 14,000. That is below
    `SCORE_KNOWN_WIN` (an ordinary heuristic score for the hash table and pruning), and far above any real eval and
    above every draw.
  - The NNUE is not used.

**progress(pos)**, from the winner's point of view, is hand-written (`Worker::conversion_eval`):

| term | weight |
|---|---|
| material, per side | man 300, king 400 (winner minus loser) |
| men advanced towards the king row | 15 per row, both sides (winner minus loser) |
| total pieces on the board | −50 each, so exchanges are welcome when winning |
| winner's kings: king-move distance to the nearest loser piece | −40 per step |
| loser's quiet moves | −10 each |
| no-progress count r (plies since the last capture or man move) | −4·r − 3·max(0, r − 50)² (−2,664 at r = 78) |

**Repetitions in conversion mode** (your point 1). As for the referee, only a **third** occurrence of a position
(game history plus search path) is a draw, and so is reaching the no-progress limit.
- A position seen **once** before is probed and scores ±`CONV_REPEAT` = ±2000 in the winner's favour.
- That is below every line that makes progress (≥ 6,000) and above every real draw (0). When all winning moves repeat
  a position, they no longer tie with moves that give the win away.
- The loser sees −2000 for a repetition, better than −6,000 to −14,000, so it seeks repetitions, as a defender should.
- The normal search keeps its 2-fold rule.

**Won root keeps the win.** With a won root, `Engine::go` searches only the moves to children that are database
losses for the opponent, or that leave the opponent without a move. Analysis with searchmoves skips this filter. The
search already ranks these moves first; the filter covers the case where every line ends in a real draw (third
occurrence, no-progress limit), where a drawing move would otherwise tie.

**Pruning in conversion mode** (your call, measured below): reverse futility, futility, razoring and ProbCut are off.
These are the rules with a margin on the eval scale; razoring and ProbCut are off by default anyway. Kept: late move
pruning, LMR, IIR, singular extensions, ETC, the hash table, move ordering, and the repetition and no-progress rules.

**Time**: normal time management. Conversion scores are not `is_decisive`, so aspiration windows and the soft limit
work as usual.
- The forced-win exit still triggers only on a real mate score (> `SCORE_MATE_BOUND`).
- The single-move exit fires when the won root has one winning move; it is then the only move that keeps the win.
- Checked through the CLI on a 7-piece position with `go btime 60000 wtime 60000`: it stopped after 0.56 s at
  depth 32, as in normal play.

**Test setting**: `Engine::set_egdb_mtc_pieces(n)` caps the pieces for which the engine uses MTC, both in
`egdb_root_move` and in the search. The default is no cap, and the DLL and CLI never set it.

## Test harness: `tests/conversion_test`

The ctest `conversion` takes about 2 s:
- It builds 2–4-piece tables (WLD and MTC) in a temporary directory and caps the engine's MTC at 0.
- The three audit positions must be won.
- The winner must keep the win at depth 1 and 2 in every sampled position where some move gives it away (118
  positions). This uses analysis moves, so the scores decide, not the root filter.
- Repetition test (point 1): the game history makes every winning move repeat a position once; a winning move must
  still be played (70/70). Then the chosen move's position is put into the history twice more, so that move would be
  the third occurrence; another winning move must be played instead (70/70). **On master both checks fail: 55/70 and
  0/55.**
- 100 sampled 3-piece and 100 sampled 4-piece wins, played by the engine at 20k nodes per move against the
  MTC-optimal defender.

**Measurement mode** (options are listed in the file header):
- **Opponent**: an `Engine` with full MTC, which plays every decided position by `egdb_root_move`. The winner follows
  MTC downhill; the loser maximises MTC, with ties going to the first move in generation order.
- **Rules**, as the referee applies them: threefold repetition within the window since the last conversion is a draw,
  and so are 79 plies without a capture or man move.
- **"Gave away the win"**: the database value turned against the winner after one of its moves.
- **Sample**: quiet positions only (the side to move has no capture), Win or Loss, a quarter each for {black, white
  winner} × {winner, loser to move}.
- **Kinds**: kings only, men on one side, or men on both sides. They are reported separately, and `--kind both`
  restricts the sample to positions with men on both sides.
- **Reference length**: the game where both sides play MTC-optimally.
  - MTC counts plies to the next conversion, not to the end of the game, so it is neither shortest-win nor
    longest-defence play.
  - The engine often wins faster than the reference (negative excess) and defends longer than it.

**Before**: origin/master 6002841 plus only the cap setter (3 lines) and the same test file, built in a scratch
worktree. Master's search does not look inside the won region, so the before numbers do not depend on the node
budget.

Commands: `run.sh` (A–D, generated 2–5-piece tables) and `run_real.sh` (R6/R7, the real 2–7-piece database on this
Mac). Logs are in `logs/before` and `logs/after`; the tables below come from `summarize.py`.

## Choices ("your call")

### Pruning

These runs used the first progress function, without the king terms, on 3-piece wins with the MTC cap at 0, 400
positions each. The variants switch off, in conversion mode only:

| variant | 5k nodes | 20k nodes |
|---|---|---|
| all pruning on | 266/400 | 319/400 |
| off: RFP, LMP, futility, LMR, IIR | 316/400 | 393/400 |

Runs that kept one of the five rules and switched the other four off all fell in between: 288–326 at 5k and
370–397 at 20k. Keeping IIR was the worst. With the final progress function, still at the MTC cap 0, the
differences mostly vanish. Below are 1000 positions each at 2k nodes on 3-piece wins, with two seeds:

| switched off | won |
|---|---|
| nothing | 804 (seed 11 only) |
| RFP | 859, 856 |
| LMP | 787, 796 |
| futility | 796, 776 |
| RFP + LMP | 840, 841 |
| RFP + futility | 863, 856 |
| LMP + futility | 786, 762 |
| RFP + LMP + futility | 854, 824 |
| RFP + LMP + futility + LMR + IIR | 859 (seed 11 only) |

At 20k nodes, all variants were within one game of 1000/1000 on 3- and 4-piece wins, and between 994 and 998 of
1000 on 5-piece wins (cap 4): 998 with everything on, 997 with RFP/LMP/futility off, 994 with all five off. Conclusions:
- Reverse futility is the rule that hurts. It trusts a coarse static value at low depth, so the engine misses king
  traps.
- Futility shows no effect either way. I switched it off as well, because its margin is on the wrong scale.
- Razoring and ProbCut use the same kind of margin and are off by default anyway.
- LMP, LMR and IIR are kept, because switching them off bought nothing measurable.

### Progress function

These runs had all pruning off, 400 positions, 5k nodes, the MTC cap at 0, and are given as won 3-piece / 4-piece:

| progress | won |
|---|---|
| material, advancement, pieces, no-progress | 316 / 387 |
| + `Evaluator::linear` (clamped ±1000) | 357 / 391 |
| + king distance and loser mobility | 394–400 / 393–396 |
| + both | 392–399 / 394–397 |

- These ranges cover the weights tried for king distance / loser mobility: 20/0, 20/10, 40/0, 40/10, 40/25, 60/10,
  80/10. With distance weight 5 or 10 the 3-piece result dropped to 354–391.
- All of them lie within the noise (±3 games). I chose 40/10, from the middle of that range.
- The linear evaluator adds nothing on top of the hand-written terms, and its weights change with training, so it is
  not used.
- King weight 450 instead of 400 (so crowning an advanced man gains more) was tried on the final version. Results:
  - real 7-piece wins: 180 vs 188 of 200 at 5k nodes, 195 vs 194 at 20k;
  - 5-piece wins: 389 vs 384 of 400 at 5k.
  That is no clear change, so it stays at 400.
- The no-progress penalty and the other material weights were not tuned.

## Results

A–D use generated tables (`run.sh`); R6 and R7 use the real database on this Mac (`run_real.sh`).
- Failures are shown in the logs (one FEN per line).
- Columns: games won by the engine before and after, failure causes after, mean game length, and engine minus
  reference length (median, p90, max).
- The last column splits the after result by kind.
- The defend tables count games the MTC-optimal attacker won, which should be all of them, and how long the engine
  survived.

### A: convert 3- and 4-piece wins (MTC cap 0 and 2), D: by node budget

| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | mean plies before / after / reference | after: engine - reference (median, p90, max) | by kind before → after (kings / men one side / men both sides) |
|---|---|---|---|---|---|---|---|---|---|
| A_cap0 | 3 | 0 | 5k | 155/400 | 400/400 | - | 44.8 / 22.2 / 24.7 | 0, 22, 52 | 3→42/42 / 75→205/205 / 77→153/153 |
| A_cap0 | 4 | 0 | 5k | 218/400 | 398/400 | repetition 1, no progress 1 | 34.7 / 13.1 / 24.4 | -6, 0, 74 | 7→24/24 / 92→188/189 / 119→186/187 |
| A_cap0 | 3 | 0 | 20k | 155/400 | 400/400 | - | 44.8 / 19.6 / 24.7 | 0, 12, 52 | 3→42/42 / 75→205/205 / 77→153/153 |
| A_cap0 | 4 | 0 | 20k | 218/400 | 400/400 | - | 34.7 / 11.9 / 24.4 | -8, 0, 70 | 7→24/24 / 92→189/189 / 119→187/187 |
| A_cap0 | 3 | 0 | 100k | 155/400 | 400/400 | - | 44.8 / 18.1 / 24.7 | 0, 4, 28 | 3→42/42 / 75→205/205 / 77→153/153 |
| A_cap0 | 4 | 0 | 100k | 218/400 | 400/400 | - | 34.7 / 10.8 / 24.4 | -8, 0, 38 | 7→24/24 / 92→189/189 / 119→187/187 |
| A_cap2 | 3 | 2 | 5k | 157/400 | 399/400 | no progress 1 | 44.8 / 22.2 / 24.7 | 0, 22, 48 | 3→42/42 / 75→204/205 / 79→153/153 |
| A_cap2 | 4 | 2 | 5k | 228/400 | 399/400 | no progress 1 | 34.7 / 13.4 / 24.4 | -6, 0, 82 | 7→24/24 / 98→189/189 / 123→186/187 |
| A_cap2 | 3 | 2 | 20k | 157/400 | 400/400 | - | 44.8 / 19.7 / 24.7 | 0, 12, 40 | 3→42/42 / 75→205/205 / 79→153/153 |
| A_cap2 | 4 | 2 | 20k | 228/400 | 400/400 | - | 34.7 / 12.4 / 24.4 | -8, 0, 82 | 7→24/24 / 98→189/189 / 123→187/187 |
| A_cap2 | 3 | 2 | 100k | 157/400 | 400/400 | - | 44.8 / 18.2 / 24.7 | 0, 4, 28 | 3→42/42 / 75→205/205 / 79→153/153 |
| A_cap2 | 4 | 2 | 100k | 228/400 | 400/400 | - | 34.7 / 11.5 / 24.4 | -8, 0, 38 | 7→24/24 / 98→189/189 / 123→187/187 |

### B: defend 3- and 4-piece losses against the MTC-optimal attacker

| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | mean plies before / after / reference | after: engine - reference (median, p90, max) | by kind before → after (kings / men one side / men both sides) |
|---|---|---|---|---|---|---|---|---|---|
| B_cap0 | 3 | 0 | 5k | 400/400 | 400/400 | - | 19.7 / 32.9 / 24.7 | 2, 36, 56 | 42→42/42 / 205→205/205 / 153→153/153 |
| B_cap0 | 4 | 0 | 5k | 400/400 | 400/400 | - | 19.5 / 37.1 / 24.4 | 2, 44, 80 | 24→24/24 / 189→189/189 / 187→187/187 |
| B_cap0 | 3 | 0 | 20k | 400/400 | 400/400 | - | 19.7 / 34.0 / 24.7 | 2, 38, 56 | 42→42/42 / 205→205/205 / 153→153/153 |
| B_cap0 | 4 | 0 | 20k | 400/400 | 400/400 | - | 19.5 / 39.1 / 24.4 | 4, 46, 90 | 24→24/24 / 189→189/189 / 187→187/187 |
| B_cap0 | 3 | 0 | 100k | 400/400 | 400/400 | - | 19.7 / 34.2 / 24.7 | 2, 36, 56 | 42→42/42 / 205→205/205 / 153→153/153 |
| B_cap0 | 4 | 0 | 100k | 400/400 | 400/400 | - | 19.5 / 40.1 / 24.4 | 4, 48, 106 | 24→24/24 / 189→189/189 / 187→187/187 |

### C: 5-piece wins (generated tables, MTC cap 4 or 0), D: by node budget

`C_cap4`: all kinds. `C_cap4_men`: men on both sides only. `C_cap0_men`: 4 and 5 pieces, men on both sides, no MTC at all.

| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | mean plies before / after / reference | after: engine - reference (median, p90, max) | by kind before → after (kings / men one side / men both sides) |
|---|---|---|---|---|---|---|---|---|---|
| C_cap0_men | 4 | 0 | 5k | 264/400 | 398/400 | repetition 1, no progress 1 | 30.8 / 13.2 / 24.1 | -4, 2, 44 | - / - / 264→398/400 |
| C_cap0_men | 5 | 0 | 5k | 244/400 | 396/400 | repetition 1, no progress 3 | 36.8 / 20.1 / 32.7 | -8, 4, 102 | - / - / 244→396/400 |
| C_cap0_men | 4 | 0 | 20k | 264/400 | 399/400 | no progress 1 | 30.8 / 11.6 / 24.1 | -4, 0, 58 | - / - / 264→399/400 |
| C_cap0_men | 5 | 0 | 20k | 244/400 | 399/400 | no progress 1 | 36.8 / 18.9 / 32.7 | -10, 2, 134 | - / - / 244→399/400 |
| C_cap0_men | 4 | 0 | 100k | 264/400 | 400/400 | - | 30.8 / 10.6 / 24.1 | -6, 0, 46 | - / - / 264→400/400 |
| C_cap0_men | 5 | 0 | 100k | 244/400 | 400/400 | - | 36.8 / 16.2 / 32.7 | -12, 0, 132 | - / - / 244→400/400 |
| C_cap4 | 5 | 4 | 5k | 322/400 | 384/400 | repetition 1, no progress 15 | 30.8 / 32.2 / 30.7 | 0, 28, 104 | 4→8/13 / 95→120/127 / 223→256/260 |
| C_cap4 | 5 | 4 | 20k | 322/400 | 399/400 | no progress 1 | 30.8 / 30.9 / 30.7 | 0, 34, 122 | 4→12/13 / 95→127/127 / 223→260/260 |
| C_cap4 | 5 | 4 | 100k | 322/400 | 400/400 | - | 30.8 / 29.0 / 30.7 | 0, 24, 80 | 4→13/13 / 95→127/127 / 223→260/260 |
| C_cap4_men | 5 | 4 | 5k | 333/400 | 395/400 | no progress 5 | 33.5 / 33.2 / 34.4 | 0, 26, 130 | - / - / 333→395/400 |
| C_cap4_men | 5 | 4 | 20k | 333/400 | 398/400 | repetition 1, no progress 1 | 33.5 / 32.7 / 34.4 | 0, 26, 152 | - / - / 333→398/400 |
| C_cap4_men | 5 | 4 | 100k | 333/400 | 400/400 | - | 33.5 / 31.9 / 34.4 | 0, 24, 156 | - / - / 333→400/400 |

### C: defend 5-piece losses (MTC cap 4)

| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | mean plies before / after / reference | after: engine - reference (median, p90, max) | by kind before → after (kings / men one side / men both sides) |
|---|---|---|---|---|---|---|---|---|---|
| C_cap4 | 5 | 4 | 5k | 400/400 | 400/400 | - | 26.8 / 47.0 / 30.7 | 8, 54, 84 | 13→13/13 / 127→127/127 / 260→260/260 |
| C_cap4 | 5 | 4 | 20k | 400/400 | 400/400 | - | 26.8 / 49.5 / 30.7 | 12, 58, 92 | 13→13/13 / 127→127/127 / 260→260/260 |
| C_cap4 | 5 | 4 | 100k | 400/400 | 400/400 | - | 26.8 / 50.6 / 30.7 | 12, 60, 102 | 13→13/13 / 127→127/127 / 260→260/260 |

### Real database, 6 and 7 pieces (men on both sides)

#### R6: 6-piece wins, engine MTC capped at 4, MTC-optimal opponent

| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | mean plies before / after / reference | after: engine - reference (median, p90, max) | by kind before → after (kings / men one side / men both sides) |
|---|---|---|---|---|---|---|---|---|---|
| R6_cap4 | 6 | 4 | 5k | 345/400 | 397/400 | repetition 1, no progress 2 | 30.3 / 27.9 / 34.5 | -2, 22, 78 | - / - / 345→397/400 |
| R6_cap4 | 6 | 4 | 20k | 345/400 | 400/400 | - | 30.3 / 26.1 / 34.5 | -4, 20, 68 | - / - / 345→400/400 |
| R6_cap4 | 6 | 4 | 100k | 345/400 | 400/400 | - | 30.3 / 25.5 / 34.5 | -4, 16, 68 | - / - / 345→400/400 |

#### R6: 6-piece losses defended against the MTC-optimal attacker

| test | pieces | MTC cap | nodes/move | before: won | after: won | after failures | mean plies before / after / reference | after: engine - reference (median, p90, max) | by kind before → after (kings / men one side / men both sides) |
|---|---|---|---|---|---|---|---|---|---|
| R6_cap4 | 6 | 4 | 5k | 400/400 | 400/400 | - | 30.4 / 56.2 / 34.5 | 18, 62, 100 | - / - / 400→400/400 |
| R6_cap4 | 6 | 4 | 20k | 400/400 | 400/400 | - | 30.4 / 58.8 / 34.5 | 22, 66, 100 | - / - / 400→400/400 |
| R6_cap4 | 6 | 4 | 100k | 400/400 | 400/400 | - | 30.4 / 58.7 / 34.5 | 20, 66, 112 | - / - / 400→400/400 |

#### R7: 7-piece wins with the real MTC (6 pieces)

The engine has no cap. There is no MTC for 7 pieces, so above 6 pieces the opponent is this branch's conversion search
at 100k nodes per move; from 6 pieces down it plays MTC-optimally. The sample is 200 positions with men on both sides.
- There is no before column: in master's binary the opponent would be master's search, whose defence collapses (it
  wins all 200 against itself), so the numbers would not be comparable.
- "Reference" is opponent against opponent. One reference game was not won.

| nodes/move | converted | failures | mean plies (engine / reference) | engine − reference (median, p90, max) |
|---|---|---|---|---|
| 5k | 188/200 | 10 no progress, 2 repetition | 62.8 / 58.3 | 0, 36, 146 |
| 20k | 194/200 | 5 no progress, 1 repetition | 60.2 / 58.3 | 0, 20, 128 |
| 100k | 199/200 | 1 no progress | 57.5 / 58.3 | — (same search as the opponent) |

- The failures are long technical endings, typically 4 kings against 3 that need exchanges deeper than depth 9–13.
- `B:W9,12,K17,28:B8,23,K29` fails at 20k, wins in 256 plies at 100k and in 40 plies at 1M.
- Engine as defender: it survived 50.9 / 53.7 plies on average at 5k / 20k, against 58.3 for the 100k-node defender;
  the attacker won every game. At 100k both sides run the same search, so that run is self-play.

### The 16 seven-piece positions of `missed_wins.txt`

These are the only ones of the 362 that this Mac can probe. The engine plays the winning side with real MTC (6
pieces). The defender is this branch's conversion search at 1M nodes per move above 6 pieces and MTC-optimal from 6
pieces down.

| engine nodes/move | won | lengths (plies) |
|---|---|---|
| 100k | 15/16 | 1 no-progress draw (`W:WK3,11,17,29:B12,18,K26`, 154 plies) |
| 1M | 16/16 | 27–175 |

Master won the same 16 against master's own defender, but that comparison is meaningless: master's defender gives
every losing move the same score and plays the first one, so it collapses.

### E: bench, ctest, TSan

- `regicide bench`: **7,677,736**, identical to master 6002841.
- ctest: 17/17 passed, including the new `conversion` test.
- TSan (RelWithDebInfo, `-fsanitize=thread`, 2 search threads, Lazy SMP sharing the hash table): no reports.
  - `conversion_test --threads 2 --sample 40 --nodes 5000 --role both` converted and defended all 160 games
    (`logs/tsan_2threads.txt`).
  - Eight real 7-piece wins at 20k nodes (`logs/tsan_2threads_real7.txt`): also clean.

## Open points

1. **The real check is still to come.** Run on Windows (8-piece WLD and 6-piece MTC):
   - Build `t3/wld-conversion` (714502c or later) in Release and run `ctest`. Also run `egdb_test --data` against
     the 8-piece directory.
   - Synthetic check with the real 8-piece tables:
     `conversion_test --dir ~/regicide-data/egdb --pieces 7,8 --kind both --sample 200 --cap 32 --nodes 100000
     --oracle-nodes 1000000 --role both`.
     There is no MTC for 7–8 pieces, so the defender above 6 pieces is this branch's conversion search at 1M nodes;
     the reference length is not MTC-optimal there.
   - The 362 positions of `experiments/booklab/missed_wins.txt`:
     - with `conversion_test --dir <db> --cap 32 --nodes 1000000 --oracle-nodes 1000000 --fen "<FEN>"` for each one;
     - and, as you plan, against the original opponents with the DLL (`regicide_cb`) at the match time control.
     Target: at least 95% converted.
   - Watch the engine's speed at 8 pieces: every node in conversion mode is probed, and 8-piece probes page in from
     a 9.7 GB set.
2. **CheckerBoard return code.**
   - `getmove` returns `CB_WIN`/`CB_LOSS` only for scores ≥ `SCORE_KNOWN_WIN`. A 7–8-piece won root used to score
     `SCORE_DB_WIN - ply` (→ `CB_WIN`); it now scores about ±10,000 (→ `CB_UNKNOWN`) until the search reaches the
     MTC region or a mate.
   - The GUI and `info` lines also show these scores as about 100 men.
   - Our match runner acts only on draw claims. If something relies on `CB_WIN`, `getmove` could report the root's
     database value instead.
3. **At very small budgets, king endings can still fail.** This shows at 5k nodes, mostly kings-only or men-on-one-side
   positions with the defender's kings in a double corner:
   - 5-piece cap 4: 15 of 400 fail at 5k, 1 at 20k, 0 at 100k. The technique needs more than depth 6–8.
   - Real games search hundreds of thousands to millions of nodes per move, but a GUI blitz setting could get close.
4. **Not tuned**: the no-progress penalty shape, the material and advancement weights, `CONV_REPEAT`. The tables
   show no failure that points at them.
5. **The hash table** keys do not include the no-progress count, while the conversion eval and the draw rules depend
   on it. This was already true for the no-progress draw. Fine in all tests, but it is a source of noise near the
   limit.
6. **Defender quality** (B) is measured only against MTC-optimal attackers on small tables. The loser now survives
   longer than master and longer than the MTC reference, and it seeks repetitions.
