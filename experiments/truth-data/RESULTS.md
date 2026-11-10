# Truth data: teach the net the endgame database's results (Windows box, 2026-11-09/10)

**Hypothesis.** The net is trained on the wrong targets and partly on the wrong positions.
- datagen never ran with the endgame database, so every game result and search score in the ~295M training positions
  comes from play without it;
- in real games the 8-piece database is always loaded and the search probes it first at every non-root node with
  ≤ 8 pieces, so the net is never evaluated there, yet those positions are a large part of the training set;
- datagen drops positions with decisive scores, which with a database would be the most valuable ones.

**Plan (this phase):** E0 census and pilot; E1 n8a = n6c's recipe without ≤ 8-piece records; E2 n8b = E1 with game
results from the database; E3 n8c = E2 with re-searched scores (proven ones exact). Every net is matched against n6c
with the standard setup.

**Status (2026-11-10 08:30):** Phase 1 done: **n8bn (E2: database-relabelled game results, ≥ 9-piece records, n6c's K)
beats n6c by +4.5 [+0.5, +8.6] at 0.1 s/move and +4.5 [+1.6, +7.4] at 0.5 s/move.** Phase 2: P1 done (n8bn vs Cake
and KingsRow with the book off: no visible difference from n6c); P2/P3/P4 running (see Phase 2).

## E0: census (no training)

Tool: `truth census` (`tools/truth.cpp`) over every training file (`~/regicide-data/train`).

**Game boundaries.** datagen appends whole games in play order, so the records of a file are a sequence of games. Two
consecutive records belong to the same game unless the second cannot follow the first: a different stored result,
more pieces of either colour, or a man that moved backwards (for every row r, the number of black men on rows ≥ r never
grows, nor the number of white men on rows ≤ r). Check: this finds exactly the game counts datagen reported for gen5a
(109,330) and gen5c (109,130).

**Did any generation use a database?** No. Every file has records with 2 and 3 pieces (a game stops at the first
position the database knows), so none of gen1…gen5c was generated with a database of any size.

"Result ≠ database": for each game, the database value of its first record with ≤ 8 pieces against the stored game
result. "≥ 9-piece records relabelled": share of a file's ≥ 9-piece records whose game result E2 changes.

| File | Records | Games | ≤ 8 pieces | Games reaching ≤ 8 | Result ≠ database | of which db decisive, game drawn | db draw, game decisive | db and game opposite | ≥ 9-piece records relabelled |
|---|---|---|---|---|---|---|---|---|---|
| gen1_n10k | 9,643,008 | 96,234 | 57.4% | 90.9% | **18.40%** | 13827 | 2274 | 1 | 17.22% |
| gen1b_n10k | 29,817,546 | 297,269 | 57.4% | 90.9% | **18.49%** | 42907 | 7057 | 5 | 17.35% |
| gen1c_n10k | 29,816,510 | 297,271 | 57.4% | 90.9% | **18.34%** | 42559 | 6992 | 6 | 17.08% |
| gen2a_n10k | 30,060,997 | 298,399 | 57.1% | 89.2% | **24.37%** | 60405 | 4437 | 9 | 22.87% |
| gen2b_n10k | 29,516,347 | 288,205 | 58.7% | 91.9% | **20.57%** | 48801 | 5657 | 9 | 19.89% |
| gen3a_n10k | 30,033,050 | 285,457 | 60.5% | 93.1% | **20.91%** | 50657 | 4894 | 10 | 20.81% |
| gen3b_n10k | 29,399,508 | 280,968 | 59.5% | 92.8% | **13.07%** | 31137 | 2925 | 4 | 13.24% |
| gen3c_n10k | 28,680,073 | 279,453 | 58.7% | 93.0% | **11.65%** | 27356 | 2904 | 1 | 11.55% |
| gen3d_n10k | 28,599,457 | 279,584 | 59.0% | 93.9% | **11.13%** | 26377 | 2828 | 2 | 11.31% |
| gen4a_n10k | 28,083,233 | 273,595 | 59.7% | 94.7% | **5.52%** | 10824 | 3465 | 5 | 5.68% |
| gen4b_n10k | 5,484,032 | 53,326 | 59.6% | 94.6% | **5.52%** | 2114 | 671 | 2 | 5.71% |
| gen5a_n50k | 10,435,120 | 109,330 | 57.4% | 91.5% | **3.35%** | 2474 | 876 | 0 | 3.23% |
| gen5c_n50k | 10,399,320 | 109,130 | 57.5% | 91.6% | **3.41%** | 2536 | 874 | 0 | 3.35% |
| **training files (all but gen4b)** | 294,484,169 | 2,894,895 | 58.5% | 92.1% | 15.19% | | | | 14.84% |

Piece-count histogram (share of records, %):

| File | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 | 24 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| gen1_n10k | 0.2 | 2.3 | 3.8 | 10.7 | 13.9 | 12.8 | 13.6 | 8.1 | 7.6 | 3.8 | 4.2 | 2.2 | 3.4 | 1.8 | 2.9 | 1.5 | 2.4 | 1.2 | 1.6 | 0.7 | 0.9 | 0.2 | 0.2 |
| gen1b_n10k | 0.2 | 2.4 | 3.8 | 10.7 | 13.8 | 12.8 | 13.7 | 8.0 | 7.6 | 3.8 | 4.2 | 2.2 | 3.4 | 1.8 | 2.9 | 1.5 | 2.4 | 1.2 | 1.6 | 0.7 | 0.9 | 0.2 | 0.2 |
| gen1c_n10k | 0.2 | 2.4 | 3.9 | 10.7 | 13.8 | 12.8 | 13.6 | 8.1 | 7.6 | 3.8 | 4.2 | 2.2 | 3.4 | 1.8 | 2.9 | 1.5 | 2.4 | 1.2 | 1.6 | 0.7 | 0.9 | 0.2 | 0.2 |
| gen2a_n10k | 0.1 | 2.1 | 4.4 | 10.3 | 13.4 | 12.5 | 14.3 | 8.5 | 7.7 | 3.8 | 4.2 | 2.2 | 3.3 | 1.7 | 2.7 | 1.6 | 2.4 | 1.3 | 1.6 | 0.7 | 0.9 | 0.2 | 0.2 |
| gen2b_n10k | 0.2 | 1.3 | 5.5 | 10.3 | 14.3 | 14.3 | 12.8 | 7.1 | 6.9 | 3.4 | 3.9 | 2.0 | 3.0 | 1.7 | 2.8 | 1.7 | 2.7 | 1.4 | 2.0 | 0.9 | 1.2 | 0.3 | 0.2 |
| gen3a_n10k | 0.2 | 1.0 | 4.7 | 11.5 | 15.7 | 14.4 | 13.1 | 7.0 | 6.6 | 3.2 | 3.6 | 1.9 | 2.7 | 1.7 | 2.6 | 1.6 | 2.6 | 1.5 | 1.9 | 0.9 | 1.2 | 0.3 | 0.2 |
| gen3b_n10k | 0.1 | 1.3 | 5.5 | 13.2 | 13.5 | 12.7 | 13.3 | 7.2 | 6.5 | 3.2 | 3.6 | 2.1 | 2.7 | 1.7 | 2.7 | 1.7 | 2.7 | 1.6 | 2.1 | 1.0 | 1.2 | 0.3 | 0.2 |
| gen3c_n10k | 0.1 | 1.1 | 5.9 | 12.1 | 13.5 | 13.4 | 12.8 | 7.2 | 6.7 | 3.4 | 4.0 | 2.1 | 2.9 | 1.7 | 2.6 | 1.7 | 2.7 | 1.5 | 2.0 | 1.0 | 1.2 | 0.3 | 0.2 |
| gen3d_n10k | 0.1 | 0.6 | 6.3 | 11.7 | 14.2 | 13.1 | 12.9 | 7.3 | 6.8 | 3.3 | 3.8 | 2.1 | 2.8 | 1.7 | 2.7 | 1.7 | 2.6 | 1.6 | 2.0 | 0.9 | 1.2 | 0.3 | 0.2 |
| gen4a_n10k | 0.1 | 0.9 | 5.4 | 9.6 | 18.1 | 12.7 | 12.9 | 6.7 | 6.8 | 3.3 | 3.8 | 1.9 | 2.9 | 1.6 | 2.8 | 1.6 | 2.6 | 1.5 | 2.0 | 0.9 | 1.1 | 0.3 | 0.2 |
| gen4b_n10k | 0.1 | 0.9 | 5.2 | 9.7 | 18.2 | 12.7 | 12.9 | 6.8 | 6.8 | 3.3 | 3.8 | 2.0 | 2.9 | 1.6 | 2.8 | 1.7 | 2.6 | 1.5 | 2.0 | 0.9 | 1.1 | 0.3 | 0.2 |
| gen5a_n50k | 0.0 | 0.2 | 5.3 | 7.5 | 18.4 | 11.3 | 14.6 | 7.0 | 7.5 | 3.4 | 4.2 | 2.1 | 3.1 | 1.8 | 2.8 | 1.7 | 2.7 | 1.5 | 2.1 | 0.9 | 1.1 | 0.3 | 0.2 |
| gen5c_n50k | 0.0 | 0.2 | 5.3 | 7.5 | 18.2 | 11.6 | 14.6 | 6.9 | 7.5 | 3.4 | 4.2 | 2.1 | 3.2 | 1.8 | 2.8 | 1.7 | 2.7 | 1.5 | 2.1 | 1.0 | 1.1 | 0.3 | 0.2 |

**Training files together** (gen1…gen5c without gen4b, 294.5M records): **≤ 8 pieces 58.5%**, 9–12 pieces 21.9%,
13–16 pieces 9.6%, 17+ pieces 9.9%. So n6c spent more than half of its training data on positions the engine never
evaluates with the net.

**Label noise falls with every generation:** 18–24% of games (gen1–gen3a) disagree with the database, 11–13% (gen3b–d),
5.5% (gen4) and 3.4% (gen5, 50k nodes). Almost all disagreements are a database win that the game drew (the self-play
failed to convert); 0–10 games per file had the opposite result. Disagreements in the other direction (database draw,
game decisive) are 14% of them in gen1 and 24–26% in gen4a and gen5.

## E0: pilot (200k records with 9–14 pieces, searched with the 8-piece database)

Tool: `truth pilot`: 200,000 uniformly random records with 9–14 pieces from all twelve training files, each searched
with the current engine (master fcf6acb, built-in n6c, no history, fresh hash) and the 8-piece database at 10k, 50k
and 200k nodes. "Proven": a decisive score (database win/loss or a forced win). "Stored result wrong": the game
result in the file disagrees with the proof at 200k; "E2 result wrong": the same after `truth relabel`.

records 200000 (14092 from 50k-node files)  budgets 10000, 50000, 200000

### all files
| pieces | records | proven @10000 | proven @50000 | proven @200000 | stored score wrong sign (of proven @200000) | stored score 0 | stored result wrong | E2 result wrong |
|---|---|---|---|---|---|---|---|---|
| 9 | 54955 | 64.9% | 72.9% | 77.9% | 0.0% (1) | 0.0% | 22.0% | 5.4% |
| 10 | 52197 | 29.1% | 35.8% | 40.1% | 0.1% (12) | 0.0% | 13.9% | 3.3% |
| 11 | 25808 | 25.2% | 39.9% | 51.5% | 0.0% (2) | 0.0% | 13.3% | 1.9% |
| 12 | 29231 | 7.7% | 13.9% | 20.5% | 0.0% (1) | 0.0% | 13.4% | 2.2% |
| 13 | 15446 | 6.4% | 13.9% | 24.1% | 0.0% (0) | 0.0% | 9.7% | 1.0% |
| 14 | 22363 | 1.5% | 3.4% | 6.4% | 0.1% (1) | 0.0% | 14.0% | 2.5% |
| 9-14 | 200000 | 30.5% | 38.0% | 44.1% | 0.0% (17) | 0.0% | 17.5% | 3.9% |

### 10k-node files (gen1-gen4a)
| pieces | records | proven @10000 | proven @50000 | proven @200000 | stored score wrong sign (of proven @200000) | stored score 0 | stored result wrong | E2 result wrong |
|---|---|---|---|---|---|---|---|---|
| 9 | 51366 | 64.7% | 72.7% | 77.7% | 0.0% (1) | 0.0% | 23.2% | 5.6% |
| 10 | 48337 | 29.1% | 35.8% | 40.1% | 0.1% (12) | 0.0% | 14.8% | 3.5% |
| 11 | 24021 | 25.3% | 40.0% | 51.6% | 0.0% (2) | 0.0% | 14.1% | 2.0% |
| 12 | 27081 | 7.9% | 14.1% | 20.7% | 0.0% (1) | 0.0% | 14.2% | 2.3% |
| 13 | 14348 | 6.5% | 14.1% | 24.3% | 0.0% (0) | 0.0% | 10.3% | 1.1% |
| 14 | 20755 | 1.5% | 3.5% | 6.5% | 0.1% (1) | 0.0% | 14.6% | 2.6% |
| 9-14 | 185908 | 30.5% | 38.1% | 44.2% | 0.0% (17) | 0.0% | 18.5% | 4.1% |

### 50k-node files (gen5a, gen5c)
| pieces | records | proven @10000 | proven @50000 | proven @200000 | stored score wrong sign (of proven @200000) | stored score 0 | stored result wrong | E2 result wrong |
|---|---|---|---|---|---|---|---|---|
| 9 | 3589 | 67.9% | 75.9% | 80.7% | 0.0% (0) | 0.0% | 5.0% | 2.3% |
| 10 | 3860 | 29.2% | 36.1% | 40.7% | 0.0% (0) | 0.0% | 2.0% | 0.5% |
| 11 | 1787 | 24.5% | 38.8% | 50.2% | 0.0% (0) | 0.0% | 1.9% | 0.7% |
| 12 | 2150 | 5.7% | 11.3% | 18.3% | 0.0% (0) | 0.0% | 2.3% | 0.0% |
| 13 | 1098 | 4.8% | 11.4% | 20.8% | 0.0% (0) | 0.0% | 0.0% | 0.0% |
| 14 | 1608 | 0.7% | 2.2% | 4.1% | 0.0% (0) | 0.0% | 1.5% | 0.0% |
| 9-14 | 14092 | 29.7% | 37.0% | 42.9% | 0.0% (0) | 0.0% | 3.4% | 1.3% |

### stored score on proven-at-50000 records, in the proven winner's direction (score x sign)
| pieces | proven | median | < 50 | < 100 | < 200 | mean target at lambda 0.5, K 102 (stored result / E2) |
|---|---|---|---|---|---|---|
| 9 | 40084 | 187 | 0.1% | 4.6% | 53.0% | 0.882 / 0.921 |
| 10 | 18688 | 292 | 4.1% | 9.4% | 24.7% | 0.920 / 0.944 |
| 11 | 10299 | 269 | 0.6% | 2.4% | 32.0% | 0.922 / 0.948 |
| 12 | 4060 | 317 | 2.4% | 6.8% | 26.6% | 0.924 / 0.950 |
| 13 | 2144 | 316 | 0.4% | 1.9% | 20.6% | 0.937 / 0.959 |
| 14 | 753 | 325 | 3.3% | 6.6% | 30.1% | 0.923 / 0.947 |
| 9-14 | 76028 | 236 | 1.3% | 5.6% | 40.7% | 0.901 / 0.933 |

### proofs at smaller budgets vs the score at 200000 nodes
- proven @10000: 60954; at 200000 same sign 60953, opposite 1, score 0 0, still proven 60909
- proven @50000: 76028; at 200000 same sign 76028, opposite 0, score 0 0, still proven 75996

### non-proven records: sign of the new score vs the stored score
- @10000: 139046 non-proven, opposite sign 2699 (1.9%), mean |new - stored| 54.0
- @50000: 123972 non-proven, opposite sign 1564 (1.3%), mean |new - stored| 64.5
- @200000: 111810 non-proven, opposite sign 1072 (1.0%), mean |new - stored| 71.2

**What the pilot says.**
- **Very many 9–14-piece positions are proven.** At 50k nodes: 73% of 9-piece, 36% of 10-piece, 40% of 11-piece, 14%
  of 12- and 13-piece and 3% of 14-piece positions; 38% of all 9–14-piece records.
- **The stored search scores were not wrong in sign:** 17 of 88k proven positions had a stored score of the wrong
  sign. Their size is moderate: median +236 in the proven winner's direction, 41% below +200. Under n6c's target
  (λ 0.5, K 102) the average target of a proven win was 0.90 (0.93 with the E2 result); E3 makes it 1.0.
- **The stored game result was the wrong label:** it disagrees with the proof in 17.5% of proven 9–14-piece positions
  of the 10k files (22% at 9 pieces) and 3.4% of gen5. The E2 relabel removes most of it (3.9% and 1.3% remain: games
  in which the self-play lost a proven win between the position and its first ≤ 8-piece record).
- **Proofs are reliable:** of 60,954 positions proven at 10k nodes, 1 has the opposite sign at 200k; of 76,028 proven
  at 50k, none.
- **Non-proven positions barely move:** the new score has the opposite sign of the stored one in 1.0–1.9%.

## Nets and matches

### Round-7 matches (pending from `experiments/nnue7`; E0 asked for them first)

`r7_match.sh`'s settings: one `rff0a3d2.so` (master ff0a3d2, built-in n6c) for both sides, A with `set evalfile`,
openings4 805 × 2, 0.1 s/move, `-c 10`, hash 128, book off, 2–8 EGDB, my background jobs stopped (matchlock).

| Match | W / D / L | Pentanomial [LL LD DD/WL WD WW] | Elo [95% CI] | LOS |
|---|---|---|---|---|
| n7b (+gen5c prefix ×3, 60 ep) vs n6c | 263 / 1072 / 275 | [0 50 717 38 0] | −2.6 [−6.6, +1.4] | 10.0% |
| n7a (λ 0.4) vs n6c | 269 / 1071 / 270 | [0 45 716 44 0] | −0.2 [−4.2, +3.8] | 45.8% |
| n6d (λ 0.3, round 6b) vs n6c | 267 / 1066 / 277 | [0 42 731 32 0] | −2.2 [−5.8, +1.5] | 12.2% |

n7b does not win, so **n6c stays the reference**.
- n7a's match overlapped by about 4 minutes with the end of my n8bk training run (2 of 24 cores and the GPU; the
  same load on both sides). From then on a match never starts while one of my training runs is going.

### E1–E3 nets

**Recipe:** n6c's, unless stated: v1 1024 → 32 → 32, 8 buckets; the same files (gen1…gen4a + gen5a ×3); 40 × 100M
positions, lr 1e-3 → 1e-5, batch 16384, wd 0.01, 2 QAT epochs, λ 0.5; trainer of this branch, one GPU lane with
1 torch + 1 loader thread. All exports are exact (`logs/export_checks.txt`).

**Matches:** one `libregicide_cb.so` of this branch (engine = master fcf6acb, built-in n6c) for both sides, A with
`set evalfile`; otherwise r7's settings (openings4 805 × 2, 0.1 s/move, `-c 10`, hash 128, book off, 2–8 EGDB, my
background jobs stopped, timing lock).

**K matters here.** The trainer fits K (the score scale of the target) to the data. Leaving out ≤ 8-piece records
refits it from n6c's 102.4 to 77.4. The database-relabelled results correlate much better with the scores and refit
it to 43.4. A smaller K shrinks the eval (mean |eval| on ≥ 9-piece positions: n6c 142.9, n8a 124.1, n8b 110.7), which
moves every pruning margin of the search. So each step was also trained at a fixed K (the "k" nets).

| Net | Data | min pieces | K | Train positions | final valid loss (int)¹ |
|---|---|---|---|---|---|
| n8a (E1) | n6c's files | 9 | 77.4 (refit) | 124.0M | 0.456206 |
| n8ak (E1, K fixed) | n6c's files | 9 | 102.4 (n6c's) | 124.0M | 0.484012 |
| n8b (E2) | relabelled (`*_e2.bin`) | 9 | 43.4 (refit) | 124.0M | 0.350709 |
| n8bk (E2, K fixed) | relabelled | 9 | 77.4 (n8a's) | 124.0M | 0.409797 |
| n8bn (E2, K fixed) | relabelled | 9 | 102.4 (n6c's) | 124.0M | 0.443396 |
<!-- E3NETS -->

¹ On each net's own validation targets (different K and labels), so not comparable between nets.

| Match (0.1 s/move, 805 pairs) | W / D / L | Pentanomial [LL LD DD/WL WD WW] | Elo [95% CI] | LOS |
|---|---|---|---|---|
| n8a (E1, K 77.4 refit) vs n6c | 264 / 1067 / 279 | [0 52 716 37 0] | −3.2 [−7.2, +0.7] | 5.6% |
| n8ak (E1, K 102.4) vs n6c | 268 / 1078 / 264 | [0 33 735 37 0] | +0.9 [−2.7, +4.4] | 68.4% |
| n8b (E2, K 43.4 refit) vs n6c | 273 / 1069 / 268 | [0 36 728 41 0] | +1.1 [−2.6, +4.8] | 71.6% |
| n8bk (E2, K 77.4) vs n6c | 283 / 1060 / 267 | [0 42 705 58 0] | +3.5 [−0.8, +7.7] | 94.5% |
| **n8bn (E2, K 102.4) vs n6c** | 276 / 1079 / 255 | [0 36 712 57 0] | **+4.5 [+0.5, +8.6]** | **98.6%** |
<!-- E3MATCHES -->

### Confirmation at 0.5 s/move: n8bn

Same engine and settings, 0.5 s/move, openings4's 805 pairs in four chunks (each run held the timing lock < 15 min;
other agents' runs in between; my background jobs stopped during each chunk). Pooled with `jobs/pool_dirs.py`.

| Chunk | Pairs | W / D / L | Pentanomial | Elo [95% CI] |
|---|---|---|---|---|
| a | 207 | 65 / 290 / 59 | [0 1 199 7 0] | +5.0 [+0.4, +9.6] |
| b | 205 | 82 / 252 / 76 | [0 5 189 11 0] | +5.1 [−1.5, +11.7] |
| c | 197 | 52 / 294 / 48 | [0 4 185 8 0] | +3.5 [−2.4, +9.5] |
| d | 196 | 68 / 261 / 63 | [0 3 185 8 0] | +4.4 [−1.3, +10.2] |
| **pooled** | **805** | **267 / 1097 / 246** | **[0 13 758 34 0]** | **+4.5 [+1.6, +7.4]**, LOS 99.9% |

**n8bn is confirmed: +4.5 [+1.6, +7.4] at 0.5 s/move**, the same size as at 0.1 s (+4.5 [+0.5, +8.6]). The longer
time control has fewer decisive pairs (47 of 805 against 93), and 34 of them are n8bn's.

### Database-truth metric

Held-out set: the 2% validation tails (never trained on by any net here, n6c or n7b) of gen5a, gen5c and gen4a, their
9–12-piece records searched at 50k nodes with the 8-piece database; the proven ones are kept (`tails/`).
The eval is the engine's exact integer eval (`nnue.forward_int`), side to move's view. "Mean abs eval" is on 100k
≥ 9-piece positions of the shared validation set (eval scale).

proven positions 96357 (wins 49906, losses 46451); by pieces: 9: 51785, 10: 25641, 11: 13606, 12: 5325
| net | sign agrees | 9 | 10 | 11 | 12 | proven losses > -30 | proven wins < +30 | mean eval wins / losses | mean abs eval (>= 9 pieces, 100000 general positions) |
|---|---|---|---|---|---|---|---|---|---|
| n4c | 99.68% | 99.95% | 99.43% | 99.50% | 98.78% | 502 (1.08%) | 740 (1.48%) | 294 / -315 | 149.6 |
| n6c | 99.78% | 99.95% | 99.61% | 99.65% | 99.27% | 409 (0.88%) | 474 (0.95%) | 307 / -309 | 142.9 |
| n7a | 99.76% | 99.95% | 99.57% | 99.67% | 99.15% | 305 (0.66%) | 605 (1.21%) | 293 / -305 | 136.1 |
| n7b | 99.77% | 99.95% | 99.61% | 99.63% | 99.19% | 311 (0.67%) | 609 (1.22%) | 300 / -314 | 139.7 |
| n8a | 99.78% | 99.95% | 99.57% | 99.72% | 99.29% | 515 (1.11%) | 491 (0.98%) | 263 / -272 | 124.1 |
| n8ak | 99.78% | 99.96% | 99.60% | 99.68% | 99.21% | 288 (0.62%) | 481 (0.96%) | 299 / -328 | 147.2 |
| n8b | 99.82% | 99.96% | 99.70% | 99.69% | 99.34% | 385 (0.83%) | 650 (1.30%) | 221 / -223 | 110.7 |
| n8bk | 99.82% | 99.96% | 99.72% | 99.71% | 99.27% | 269 (0.58%) | 463 (0.93%) | 294 / -303 | 145.7 |
| n8bn | 99.83% | 99.97% | 99.72% | 99.72% | 99.27% | 222 (0.48%) | 358 (0.72%) | 339 / -337 | 166.7 |

## Phase 2

**Builds.** All self-play net matches of both phases use one `libregicide_cb.so` of this branch at **fcf6acb** (master
fcf6acb, bench 7,712,020) for both sides (`rtruth.so`). P1 uses the ponder agent's DLL `ponder-df7780a` for every run
(n6c and n8bn alike). On 2026-11-10 the branch merged master 6002841 (bench **7,677,736**, ctest 16/16); from then on
rescore and datagen use the merged code. Self-play comparisons were not moved to the new build, so no comparison mixes
builds.

### P1: n8bn against the real opponents (config A, book off)

The ponder agent's config A (`jobs/ponder/run_p.sh`): DLL `ponder-df7780a`, threads 1, ponder 0, hash 128, `-c 8`, book
off, ACF 156 deck × 2 colours = 312 games, 0.5 s/move. Cake and KingsRow with their books and the KingsRow 2–8 WLD
database. n8bn through `set evalfile` (checked by hand: `reply 1 nnue`). Background as in the reference runs: my
extra threads stopped during these runs (load 9–10). The n6c Cake runs are the ponder agent's `pt-s2-a1-cake` and
`pt-s2-a2-cake`. No config-A n6c run against KingsRow existed, so I played one (`tr-n6c-kingsrow`).

| Run | W / D / L | Elo [95% CI] |
|---|---|---|
| n8bn vs Cake, run 1 | 0 / 307 / 5 | −5.6 [−10.4, −0.8] |
| n8bn vs Cake, run 2 | 0 / 307 / 5 | −5.6 [−10.4, −0.8] |
| n6c vs Cake (ponder agent, 2 runs) | 0 / 307 / 5 each | −5.6 [−10.4, −0.8] each |
| n8bn vs KingsRow | 1 / 311 / 0 | +1.1 [−1.1, +3.3] |
| n6c vs KingsRow (reference) | 1 / 309 / 2 | −1.1 [−4.9, +2.7] |

**Cake, ballots lost** (each run plays every ballot once with each colour):
- **n8bn:** #34 Twilight Zone (black, both runs), #6 Inferno (black, both), #74 (black, both), #153 Black Widow (white, both),
  #96 Skull-Cracker (black, run 1), #112 (white, run 2).
- **n6c:** #7 (black, both), #62 Gemini I (black, both), #153 Black Widow (white, both), #5 Dreaded Edinburgh, #89 (black,
  run 1), #64 (black), #129 (white, run 2).
- Only #153 is lost by both nets. n8bn no longer loses #7 and #62 (n6c's repeated losses), but loses #34, #6 and #74
  in both runs.

**KingsRow:**
- n8bn won #50 (white).
- n6c won #122 (black) and lost #16 Wilderness II and #68 White Doctor (black).

**Reading.** The self-play gain does not show against Cake with the book off: the same 5 losses per 312 games, on
other ballots. Against KingsRow n8bn is +2 games better than n6c, within noise.

### Review fixes (before any E5 data)

- **Incomplete searches.**
  - rescore, datagen and the pilot no longer take a search that completed no iteration as a result.
  - On top of that, the proven rule of master 5ea3189 (`tools/label.hpp`): a win is proven even from a partial
    iteration; a loss only from a complete one. A partial loss is stored unproven with `completed_score`, and counted
    (`partial losses` in the rescore and datagen logs).
- **Checks on the already rescored files.** A sentinel would map to −3000 + PROVEN, exactly like a real proven loss.
  - Guarded rescore of 300k gen4a records at the production budgets: **0 untrusted searches in 89,420**.
  - Proven losses whose previous score was above +100: **0** in every rescored file (gen5a 549,270 proven losses, gen4a's
    first 13.0M records 612,340, the tails 46,451). The symmetric control, proven wins with a previous score below −100,
    gives 1.
- **No recheck of old proven losses (lead's decision).** 1,208,061 proven losses were rescored before the partial
  rule. A cold re-search of a 200k-record sample demoted 906 of 10,569, of which only 135 were partial losses: re-search
  without the original hash state drops real proofs. gen4a from record 13,020,160 on uses the new rule.
- **Game boundaries.**
  - datagen now marks the first record of every game (stm bit 6, `GAME_START`); truth census and relabel split there
    when a file has the flags.
  - Test: two seeded K-vs-K games, 5 games written. The flags give 5; the old heuristic counted 1.
  - All E5 data has the flags (gen6b was restarted twice, discarding what the older binaries had written).

<!-- P3 -->

<!-- P4 -->

## What holds and what does not

- **Wrong targets: yes, for the game result.** 15% of all games (18–24% of gen1–gen3a) end with a result the database
  contradicts, almost always a database win that the self-play drew. On proven 9–14-piece positions the stored result
  is wrong 17.5% of the time (10k files). Replacing it with the database value (E2) is the step that gains:
  n8bn +4.5 [+0.5, +8.6] at 0.1 s and +4.5 [+1.6, +7.4] at 0.5 s/move.
- **Wrong positions: not by themselves.** Dropping the ≤ 8-piece records (58.5% of the data) is neutral at n6c's K
  (n8ak +0.9 [−2.7, +4.4]) and costs about 4 Elo if K is refitted (n8a −3.2). The net does not seem to be hurt by
  spending capacity on positions it never sees in search. Dropping them still halves the data and makes E2/E3 cleaner,
  and every gain here was measured with them dropped.
- **The stored search scores were not wrong in sign.** On positions the database search proves, the stored score had
  the wrong sign 17 times in 88k; its size was moderate (median +236). E3 can only sharpen these targets (0.93 → 1.0
  for a proven win under n6c's λ and K) and correct the 1–2% of non-proven scores whose sign changes.
- **K is a hidden variable.** The trainer refits K to the data. Database-true results correlate so well with the scores
  that the refit K drops to 43 (from 102), which shrinks the eval and lost about 3 Elo against the same data at a
  larger K (n8b +1.1 vs n8bk +3.5 vs n8bn +4.5). Any truth-labelled net should keep n6c's K (or K should be tuned
  separately).
- **The database-truth metric hardly separates nets.** Every net agrees in sign with 99.7–99.8% of the held-out proven
  9–12-piece positions; proven positions are mostly lopsided. The E2 nets at a large K do best on the tails (n8bn:
  0.48% of proven losses above −30 against n6c's 0.88%), in line with the matches, but the metric would not have
  predicted the match results on its own.

<!-- BACKGROUND -->

## Files

- `scripts/`: `census_report.py`, `pilot_report.py`, `db_truth.py`; `jobs/` (the shell jobs as run).
- Logs and tables: `logs/`.
- Data on the Windows box only: `~/regicide-data/truth/` (`*_e2.bin` relabelled files, `*_e3.bin` rescored files,
  `tails/` the held-out proven set, `seeds_real_6_40.txt`).
