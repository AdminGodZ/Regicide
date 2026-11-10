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

Determinism (same pairing twice, game by game): pending (Cake–Cake repeat and Regicide–GuiNN repeat are the last
census runs in the queue). From the 2026-11-09 targeted runs: Regicide vs Cake, same 100 games twice: 12% identical
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
| 2 | bl-r5 | Cake | h1 0 / 156 / 0, h2 running | queued |
| 2 | bl-r5 | KingsRow | h1 0 / 156 / 0, h2 queued | queued |
| 2 | bl-r5 | GuiNN | queued | queued |

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
