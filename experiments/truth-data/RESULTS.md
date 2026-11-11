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

**Status (2026-11-10 16:30):**
- **Phase 1:** n8bn (E2: database-relabelled game results, ≥ 9-piece records, n6c's K) beats n6c by +4.5 [+0.5, +8.6]
  at 0.1 s/move and +4.5 [+1.6, +7.4] at 0.5 s/move. One seed, best of the recipes tried; see "Reading the evidence".
  Its seed-2 replicate n8bn2 scores +3.0 [−0.9, +7.0].
- **Phase 2:**
  - P1: against Cake and KingsRow with the book off, no difference from n6c shown.
  - P2: E3 (n8c) shows no gain over E2.
  - P3: no sweep beats n8bn.
  - T4: **n8q** (n8bn plus 10% ≤ 8-piece records) keeps the endings sane without the database and passes both match
    conditions. It is the built-in-net candidate (commit bd1c5c0, not merged until the Book Lab games are in).
  - P5: the wide nets are slower and lose or tie at 0.1 s; their 0.5 s matches moved to the Arch laptop.
  - P4 (E5 datagen) running.

## Reading the evidence (audit, 2026-11-10)

### What the n8bn result rests on

- **One seed, best of many.** n8bn's +4.5 [+0.5, +8.6] at 0.1 s is the best of **five recipes** that existed when it
  was played (n8a, n8ak, n8b, n8bk, n8bn) and of **eleven** by now (+ n8bk130, n8bl6, n8bl7, n8p, n8p2, n8c). It comes
  from **one training seed**.
- **The 0.1 s result alone does not survive the selection:** SE ≈ 2.07 Elo, z = 2.18, one-sided p = 0.015; ×11
  (Bonferroni) gives 0.16.
- **The 0.5 s confirmation** is a second measurement at another time control, **on the same 805 openings, not a new
  opening sample.** Its games are new (searches are not deterministic), and it was taken after n8bn had been chosen.
  - Result: +4.5 [+1.6, +7.4]: SE ≈ 1.48, z = 3.04, p = 0.0012.
  - It stays significant after allowing for eleven candidates (p ≈ 0.013).
- **Training seed replicate (audit):** n8bn2 = n8bn's exact recipe with training seed 2 (n8bn: seed 1), at 0.1 s
  against n6c on the same engine and openings.

| Net | Seed | vs n6c at 0.1 s | Pentanomial |
|---|---|---|---|
| n8bn | 1 | +4.5 [+0.5, +8.6] | [0 36 712 57 0] |
| n8bn2 | 2 | +3.0 [−0.9, +7.0] | [0 37 717 51 0] |
| both seeds pooled (1,610 pairs) | | +3.8 [+0.9, +6.6], LOS 99.5% | [0 73 1429 108 0] |

- **Reading:** the second seed also lands above zero (LOS 93%). The two seeds differ by 1.5 Elo, which is inside the
  noise. The recipe's gain is real but probably nearer +3 to +4 than n8bn's single estimate.
- **What the evidence does not cover:** other openings.

### Resolution of the tests

Differences smaller than the following are not resolved. A result inside them reads "no difference shown at this
resolution", not "equal".
- One self-play test of 805 pairs at 0.1 s has a 95% CI of about ±4 Elo, so it detects a true difference of about
  **5–6 Elo** (80% power).
- Comparing two nets through their separate results against n6c combines two such errors: about **8 Elo**.
- The 312-game runs against Cake, KingsRow and GuiNN have CIs of ±2 to ±10 Elo, depending on the number of decisive
  games: about **6–17 Elo**.

| Claim | Evidence | Resolution |
|---|---|---|
| n8bn beats n6c | +4.5 (0.1 s), +4.5 (0.5 s); replicate +3.0 | shown (above) |
| K 102.4 best | K 43.4 +1.1, 77.4 +3.5, 102.4 +4.5, 130 +3.2 (each vs n6c) | no difference shown between 77, 102 and 130 at ~8 Elo |
| λ 0.5 best | λ 0.6 −0.2, 0.7 +3.0 vs n6c | no difference shown at ~8 Elo |
| E3 adds nothing to E2 | n8c +0.4 vs n6c (n8bn +4.5) | no difference shown at ~8 Elo; the direct n8c–n8bn match moved to the Arch laptop (see P2) |
| phase-balanced = n8bn | n8p vs n8bn +0.4 [−3.6, +4.5], n8p2 −0.4 [−4.2, +3.3] | no difference shown at ~5–6 Elo |
| removing ≤ 8 pieces is neutral | n8ak +0.9 [−2.7, +4.4] | no difference shown at ~5–6 Elo |
| n8q = n8bn with the database | n8q vs n8bn +1.5 [−2.1, +5.1] | no difference shown at ~5–6 Elo |
| nets do not differ vs GuiNN/Cake | 16–19 wins vs GuiNN, 5–7 losses vs Cake | no difference shown at ~6–17 Elo |

**Measurements and forecasts are kept apart.** Numbers in tables are measurements. The E5 finishing time is a forecast
and is marked as one.

### Data for re-deriving the tables (`data/`)

- `pilot_2026-11-09.csv.gz`: the E0 pilot's per-record output (old label rule).
- `pilot2_label_rule.csv.gz`: the same 200k records with the label.hpp rule.
- `heldout_proven_*_9_12.bin.gz`: the held-out proven set (DataRecords); `db_truth_per_record.csv.gz`: every net's
  eval on every record of it.
- `e5_census.txt`, `e5_seed_provenance.txt` and `seeds_real_6_30.txt.gz`: the E5 production data and where its seeds
  come from.
- `matches/ref-ponder-*`: the ponder agent's two n6c reference runs against Cake, each with a `NOTE.txt` on its origin.

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

> **Re-run with the corrected label rule (2026-11-10).** The table below was made with the first pilot, which took a
> search's score as given: a losing score from an unfinished last iteration counted as a proof. The same 200k records
> were searched again with `truth pilot` using `label_of` (`tools/label.hpp`), on the current engine (this branch with
> master 2efe11a merged). Full output: `logs/pilot2_report.md`; per-record data: `data/pilot2_label_rule.csv.gz`.
>
> | | proven @10k | proven @50k | proven @200k | stored result wrong (of proven @200k) |
> |---|---|---|---|---|
> | 9 pieces: first pilot / re-run | 64.9% / 64.3% | 72.9% / 72.6% | 77.9% / 77.6% | 22.0% / 21.9% |
> | 12 pieces | 7.7% / 7.4% | 13.9% / 13.5% | 20.5% / 20.0% | 13.4% / 13.2% |
> | 9–14 pieces | 30.5% / 30.0% | 38.0% / 37.6% | 44.1% / 43.7% | 17.5% / 17.4% |
>
> - The first pilot overstated the proven shares by 0.3–0.6 percentage points.
> - Under the rule every proof is stable: all 59,990 positions proven at 10k nodes are still proven at 200k (first pilot:
>   45 of 60,954 were not, 1 with the opposite sign).
> - The conclusions drawn from the pilot do not change.

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
- **The stored game result was the wrong label:** it disagrees with the proof in 18.5% of proven 9–14-piece positions
  of the 10k files (17.5% over all files) (22% at 9 pieces) and 3.4% of gen5. The E2 relabel removes most of it (3.9% and 1.3% remain: games
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

¹ On each net's own validation targets (different K and labels), so not comparable between nets.

| Match (0.1 s/move, 805 pairs) | W / D / L | Pentanomial [LL LD DD/WL WD WW] | Elo [95% CI] | LOS |
|---|---|---|---|---|
| n8a (E1, K 77.4 refit) vs n6c | 264 / 1067 / 279 | [0 52 716 37 0] | −3.2 [−7.2, +0.7] | 5.6% |
| n8ak (E1, K 102.4) vs n6c | 268 / 1078 / 264 | [0 33 735 37 0] | +0.9 [−2.7, +4.4] | 68.4% |
| n8b (E2, K 43.4 refit) vs n6c | 273 / 1069 / 268 | [0 36 728 41 0] | +1.1 [−2.6, +4.8] | 71.6% |
| n8bk (E2, K 77.4) vs n6c | 283 / 1060 / 267 | [0 42 705 58 0] | +3.5 [−0.8, +7.7] | 94.5% |
| **n8bn (E2, K 102.4) vs n6c** | 276 / 1079 / 255 | [0 36 712 57 0] | **+4.5 [+0.5, +8.6]** | **98.6%** |

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

**n8bn is confirmed: +4.5 [+1.6, +7.4] at 0.5 s/move**, the same size as at 0.1 s (+4.5 [+0.5, +8.6]). This is a second measurement at another time control on the **same 805 openings**, not a new opening sample (see "Reading the evidence"). The longer
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
| n7b | 99.77% | 99.95% | 99.61% | 99.63% | 99.19% | 311 (0.67%) | 609 (1.22%) | 300 / -314 | 139.7 |
| n8a | 99.78% | 99.95% | 99.57% | 99.72% | 99.29% | 515 (1.11%) | 491 (0.98%) | 263 / -272 | 124.1 |
| n8ak | 99.78% | 99.96% | 99.60% | 99.68% | 99.21% | 288 (0.62%) | 481 (0.96%) | 299 / -328 | 147.2 |
| n8b | 99.82% | 99.96% | 99.70% | 99.69% | 99.34% | 385 (0.83%) | 650 (1.30%) | 221 / -223 | 110.7 |
| n8bk | 99.82% | 99.96% | 99.72% | 99.71% | 99.27% | 269 (0.58%) | 463 (0.93%) | 294 / -303 | 145.7 |
| n8bn | 99.83% | 99.97% | 99.72% | 99.72% | 99.27% | 222 (0.48%) | 358 (0.72%) | 339 / -337 | 166.7 |
| n8bn2 | 99.83% | 99.95% | 99.69% | 99.76% | 99.46% | 285 (0.61%) | 295 (0.59%) | 346 / -336 | 168.6 |
| n8bk130 | 99.82% | 99.96% | 99.70% | 99.74% | 99.21% | 193 (0.42%) | 350 (0.70%) | 361 / -380 | 187.7 |
| n8bl6 | 99.83% | 99.96% | 99.72% | 99.71% | 99.32% | 273 (0.59%) | 355 (0.71%) | 347 / -368 | 181.0 |
| n8bl7 | 99.78% | 99.95% | 99.62% | 99.64% | 99.32% | 193 (0.42%) | 404 (0.81%) | 363 / -392 | 196.7 |
| n8p | 99.80% | 99.96% | 99.67% | 99.68% | 99.21% | 269 (0.58%) | 401 (0.80%) | 321 / -337 | 163.8 |
| n8p2 | 99.80% | 99.96% | 99.68% | 99.66% | 99.19% | 269 (0.58%) | 372 (0.75%) | 328 / -333 | 166.2 |
| n8c | 99.84% | 99.96% | 99.72% | 99.74% | 99.44% | 267 (0.57%) | 291 (0.58%) | 357 / -349 | 164.8 |
| n8q | 99.83% | 99.95% | 99.72% | 99.74% | 99.40% | 265 (0.57%) | 386 (0.77%) | 325 / -331 | 161.0 |
| w2032 | 99.83% | 99.97% | 99.69% | 99.73% | 99.44% | 190 (0.41%) | 522 (1.05%) | 302 / -351 | 159.8 |
| w1064 | 99.83% | 99.96% | 99.75% | 99.68% | 99.34% | 194 (0.42%) | 412 (0.83%) | 338 / -341 | 167.4 |
| w2064 | 99.86% | 99.97% | 99.75% | 99.77% | 99.49% | 182 (0.39%) | 363 (0.73%) | 315 / -346 | 162.2 |

(Per-record evals: `data/db_truth_per_record.csv.gz`; held-out set: `data/heldout_proven_*_9_12.bin.gz`. Updated 2026-11-10 with every net trained since, including n8bn2, n8q and the wide nets.)

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

**Book-mode note (lead, 2026-11-10): KingsRow ran with book mode 1 in all of these runs, not mode 3.** Every conclusion
here and below that rests on KingsRow is provisional until it is repeated with mode 3.

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

### P2: E3 (n8c)

**Net.** n8c is n8bn's recipe (E2 relabelled results, ≥ 9 pieces, K 102.4, λ 0.5, n6c's files) with **gen5a (×3) and
gen4a rescored**:
- every record with 9–16 pieces is re-searched with the 8-piece database: gen5a 50k nodes; gen4a 50k nodes at 9–12 and
  20k at 13–16;
- proven records are trained on their exact result whatever λ is.

**Rescored data:**
- gen5a: 1,144,718 proven of 3.33M searched;
- gen4a: 2,753,696 proven (1,304,428 losses, 1,449,268 wins) of 8.4M searched;
- gen4a from record 13,020,160 on was rescored under the partial-loss rule: 16,120 of 4,483,523 searches (0.36%) ended
  in a partial loss and were stored unproven with `completed_score`.

| Match (0.1 s/move, 805 pairs, `rtruth.so` fcf6acb) | W / D / L | Pentanomial | Elo [95% CI] | LOS |
|---|---|---|---|---|
| n8c (E3) vs n6c | 263 / 1086 / 261 | [0 38 727 40 0] | +0.4 [−3.3, +4.2] | 59.0% |
| n8bn (E2) vs n6c, for comparison | 276 / 1079 / 255 | [0 36 712 57 0] | +4.5 [+0.5, +8.6] | 98.6% |

**No gain from E3 over E2 shown.** n8c scores +0.4 against n8bn's +4.5 vs n6c; through n6c this comparison
resolves only about 8 Elo. The direct match n8c vs n8bn (805 pairs) was not played here; the lead moved it to the
Arch laptop (origin/t3/search-lab, "Net matches (for truth-data)").
- n8c has the best database-truth numbers of all nets: sign agreement 99.84%, proven wins below +30 0.58%.
- Its per-group loss on the E2 targets equals n8bn's (0.46488 against 0.46465).
- The sharper targets on proven positions are learnt, but they do not make the net play better than the relabelled
  results alone.
- This matches the pilot: the stored scores were already right in sign and of reasonable size on the positions the
  database search proves.

**The best net of P2 and P3 stays n8bn** (no net beats it), so there is no 0.5 s confirmation against n8bn to play.

### Opponents on a DLL with the conversion search (lead, 2026-11-10)

**Setup.** DLL `truth-c0dde2e`: this branch at c0dde2e = master 2efe11a (conversion search) merged; bench 7,677,736;
built with llvm-mingw like the ponder agent's DLLs; egdb junction to the 2–8 database. Otherwise config A as in P1
(book off, threads 1, hash 128, `-c 8`, ACF 156 × 2, 0.5 s/move; the opponents with their books and the KingsRow 8-piece
WLD database).

| Run (truth-c0dde2e, config A, book off, 312 games) | W / D / L | Elo [95% CI] |
|---|---|---|
| n8bn vs Cake | 0 / 307 / 5 | −5.6 [−10.4, −0.8] |
| n8c vs Cake | 0 / 307 / 5 | −5.6 [−10.4, −0.8] |
| n6c (built-in) vs Cake, reference | 0 / 305 / 7 | −7.8 [−13.4, −2.2] |
| n8bn vs GuiNN | 16 / 294 / 2 | +15.6 [+6.7, +24.6] |
| n8c vs GuiNN | 17 / 293 / 2 | +16.7 [+7.6, +25.9] |
| n6c (built-in) vs GuiNN, reference | 19 / 290 / 3 | +17.8 [+7.5, +28.2] |

**Reading:**
- **GuiNN:** with the conversion search all three nets beat GuiNN by +16 to +18 Elo, and no difference between the nets is shown
  (16–19 wins, 2–3 losses; these 312-game runs resolve about 6–17 Elo). The gain comes from the conversion search, not from the truth-data nets.
- **Cake:** still no wins. n8bn and n8c lose 5 games, n6c 7, which is within noise.
- **The 0/307/5 streak:** six config-A Cake runs in a row (n6c ×2 on the ponder DLL, n8bn ×3, n8c) ended 0/307/5 on
  partly different ballots. The n6c reference on the new DLL (0/305/7) breaks it, so it was chance, not a harness
  artefact.

**Ballots:**
- **Cake, n8bn on truth-c0dde2e:** lost #6 Inferno, #14, #34 Twilight Zone, #85 (black) and #153 Black Widow (white).
  #6, #34 and #153 were also lost on the ponder DLL.
- **GuiNN, n8bn:**
  - won #16 Wilderness II, #23, #34 Twilight Zone, #55, #60 Black Hole, #71 Diabolical Denny, #74, #76, #81, #86 Octopus,
    #141 and #174 Skunk (white);
  - won #22, #28, #92 and #121 (black);
  - lost #6 Inferno and #96 Skull-Cracker (black).

### P3: sweeps on the E2 data

All nets are n8bn's recipe (E2 data: database-relabelled results; ≥ 9 pieces; v1 1024; 40 × 100M; lr 1e-3 → 1e-5) with
one change each. They are matched at 0.1 s/move, 805 pairs, on `rtruth.so` (fcf6acb) for both sides.

| Net | Change from n8bn | vs n6c: W / D / L | Pentanomial | Elo vs n6c [95% CI] | vs n8bn |
|---|---|---|---|---|---|
| n8bn | (K 102.4, λ 0.5) | 276 / 1079 / 255 | [0 36 712 57 0] | **+4.5 [+0.5, +8.6]** | — |
| n8bk130 | K 130 | 273 / 1079 / 258 | [0 34 722 49 0] | +3.2 [−0.6, +7.1] | |
| n8bl6 | λ 0.6 | 268 / 1073 / 269 | [0 43 720 42 0] | −0.2 [−4.1, +3.7] | |
| n8bl7 | λ 0.7 | 278 / 1068 / 264 | [0 35 721 49 0] | +3.0 [−0.8, +6.9] | |
| n8p | phase-balanced 1/3 : 1/3 : 1/3 | 266 / 1075 / 269 | [0 39 730 36 0] | −0.6 [−4.3, +3.0] | +0.4 [−3.6, +4.5] |
| n8p2 | phase-balanced 1/4 : 1/4 : 1/2 | 278 / 1072 / 260 | [0 32 723 50 0] | +3.9 [+0.1, +7.7] | −0.4 [−4.2, +3.3] |

Earlier steps of the same family, at λ 0.5: K 43.4 +1.1, K 77.4 +3.5, K 102.4 +4.5, K 130 +3.2.

**No sweep beats n8bn.**
- K: no gain beyond 102.4 shown (K 130 +3.2).
- λ: 0.6 and 0.7 show no gain over 0.5 (0.6 has the lowest point estimate, −0.2, inside the others' intervals).
- Phase-balanced sampling: no difference from n8bn shown head to head, in either split.
- **Resolution:** an 805-pair test detects about 5–6 Elo, and a comparison through two results against n6c about
  8 Elo. None of these differences reaches that, so all read "no difference shown at this resolution".
- The differences between these nets (±2–4 Elo) are inside the CIs of single 805-pair runs.

**How phase-balanced sampling works** (`nnue.Data(groups=…, weights=…)`, trainer `--groups 9,13,17 --group-weights …`):
- For every file the kept training indices (≥ 9 pieces, outside the 2% validation tail) are split by piece count into
  9–12, 13–16 and 17–24.
- Every batch of n records takes exactly the largest-remainder split of n by the weights from each group: 16,384 →
  5,462 / 5,461 / 5,461 for 1:1:1, and 4,096 / 4,096 / 8,192 for 1:1:2.
- Within a group, records are drawn uniformly (with replacement) over all files; a file given three times counts three
  times, as in the plain sampler.
- Exact, not approximate: checked on real batches (the counts per group equal the split).
- The natural shares of the E2 training set are 53% / 23.5% / 23.6%.

**Per-group validation loss.** Set: the 2% validation tails of the E2 files, ≥ 9 pieces (2.34M records), 100k sampled per
group, the same records for every net. Loss: BCE against n8bn's target (λ 0.5, K 102.4, database-relabelled results);
also against the result alone and the search score alone. Eval: the engine's integer eval.

validation tails of 11 files, >= 9 pieces: 2343232 records; sampled per group [100000, 100000, 100000]; K 102.4, lambda 0.50

| net | 9-12 λ0.5 | 9-12 result | 9-12 score | 13-16 λ0.5 | 13-16 result | 13-16 score | 17-24 λ0.5 | 17-24 result | 17-24 score | all λ0.5 (equal weight) |
|---|---|---|---|---|---|---|---|---|---|---|
| n6c | 0.41311 | 0.34149 | 0.48474 | 0.48667 | 0.41799 | 0.55535 | 0.51270 | 0.44068 | 0.58473 | 0.47083 |
| n8bn | 0.40374 | 0.31004 | 0.49745 | 0.48081 | 0.39594 | 0.56567 | 0.50939 | 0.42561 | 0.59317 | 0.46465 |
| n8a | 0.41986 | 0.35754 | 0.48218 | 0.49001 | 0.42970 | 0.55032 | 0.51616 | 0.45321 | 0.57911 | 0.47534 |
| n8ak | 0.41182 | 0.33768 | 0.48597 | 0.48459 | 0.41169 | 0.55748 | 0.51118 | 0.43558 | 0.58677 | 0.46919 |
| n8b | 0.42005 | 0.35642 | 0.48368 | 0.49581 | 0.44164 | 0.54997 | 0.52358 | 0.46928 | 0.57789 | 0.47981 |
| n8bk | 0.40517 | 0.32379 | 0.48655 | 0.48168 | 0.40882 | 0.55454 | 0.51058 | 0.43799 | 0.58317 | 0.46581 |
| n8bk130 | 0.40634 | 0.29986 | 0.51282 | 0.48332 | 0.38565 | 0.58100 | 0.51197 | 0.41517 | 0.60878 | 0.46721 |
| n8bl6 | 0.40551 | 0.30046 | 0.51056 | 0.48199 | 0.38850 | 0.57549 | 0.51016 | 0.41751 | 0.60281 | 0.46589 |
| n8bl7 | 0.41038 | 0.29486 | 0.52590 | 0.48628 | 0.38315 | 0.58941 | 0.51370 | 0.41210 | 0.61529 | 0.47012 |
| n8p | 0.40509 | 0.31518 | 0.49499 | 0.47912 | 0.39410 | 0.56414 | 0.50780 | 0.42299 | 0.59261 | 0.46400 |
| n8p2 | 0.40618 | 0.31491 | 0.49746 | 0.48112 | 0.39646 | 0.56579 | 0.50689 | 0.41939 | 0.59439 | 0.46473 |

**Does per-group loss predict the matches?** Only roughly.
- The best match net (n8bn) has the lowest loss in 9–12 and among the lowest overall.
- n8p has the lowest overall loss (0.46400, better than n8bn on 13–16 and 17–24) but scores −0.6 vs n6c against
  n8bn's +4.5, and equal head to head.
- n8bl6 is third-lowest overall (0.46589) and the weakest of the sweeps in play.
- Nets trained at a different K (n8a, n8b) look worse here partly because this loss uses K 102.4.

So differences of 0.001 in this loss do not rank nets of the same family. Larger gaps (n6c 0.47083 against the E2 nets
0.464–0.466) do go with the match results.

### P4: E5 data with the database (running)

**What `datagen-eb1037c` is.** The old gen4b generator (10k nodes, n4c engine eb1037c, seed 54,
`~/regicide-data/train/gen4b_n10k.bin`, 5.48M records). It has been stopped (SIGSTOP, `gen4bhold`) since 2026-11-08 and
generates nothing.

**Settings** (`jobs/e5_gen.sh`, datagen of this branch):
- 8-piece database, 50k nodes/move;
- a game ends at its first ≤ 8-piece position with the database value as result, so nothing ≤ 8 is written;
- positions with a proven score are written with the PROVEN flag (`label.hpp` rule);
- score adjudication counts only proven scores, so every decisive result is exact;
- each game's first record carries GAME_START;
- half the games start from random openings (4–12 random plies), half from **seeds**: 227,871 distinct positions at
  plies 6–30 of the 25,113 match games vs Cake (11,542), KingsRow (7,801) and GuiNN (5,770) under
  `~/regicide-match/results`, plus 2–4 random plies.
- Files: `gen6a_n50k_egdb.bin` (6 base-budget threads, seed 611) and `gen6b_n50k_egdb.bin` (6 extra threads that yield
  to Book Lab's computing and to my CheckerBoard runs, seed 612).

**Throughput.** 20-minute side-by-side test, same engine, 3 threads each (`logs/e5_rate.txt`):

| Settings | Records per CPU-hour | ≥ 9-piece records per CPU-hour | Proven |
|---|---|---|---|
| gen5a style (no database, random openings, 50k nodes) | 170,929 | 74,111 | 0 (dropped) |
| E5 (database, half seeds, 50k nodes) | 154,342 | **154,342** | 10.3% |

The production run confirms it: 158k ≥ 9-piece positions per CPU-hour. So E5 gives **2.1× as many useful (≥ 9-piece)
positions per CPU-hour** as gen5a's settings.

**Composition and game ends:**
- piece counts: 9–12 42%, 13–16 34%, 17–24 24% (E2's ≥ 9-piece data: 53 / 23.5 / 23.6), so more weight on the middle
  game;
- game ends (gen6a, first 17,000 games): database 7,467, proven adjudication 9,430, draw rules 103, no legal moves 0;
- 0 incomplete searches; 2,318 partial losses stored unproven.

**Measured (2026-11-10 16:45, `data/e5_census.txt`):** 6,405,632 positions with ≥ 9 pieces in 233,290 games (gen6a 3,214,080 in 117,048 games; gen6b 3,191,552 in 116,242), game boundaries from the GAME_START flags, no record with ≤ 8 pieces. Seeds: `data/e5_seed_provenance.txt` (sha256 of the seed file, the command, the 126 PDN files).

**Forecast (not a measurement):** about 1.9M positions per hour while both processes run (12 threads × 158k per CPU-hour); 30M would be reached after about 12–13 more hours of running, i.e. around 2026-11-11 05:00–08:00 depending on pauses. Then n9a: n8q's recipe (incl. the ≤ 8-piece group) + this data ×3, against the best n8 net.

### T4: a built-in net that stays sane without the database (n8q)

**Problem (lead, 2026-11-10).** A net trained only on ≥ 9-piece records has never seen an ending. n8bn evaluates three
kings against one at 0, and with n8bn as the built-in net the engine's `search` test fails (reproduced: ctest 26/27). With
the 8-piece database loaded that never matters. Without it (the command-line engine without the 11 GB database, the
Mac builds, the engine downloaded alone) every ending would be played blind.

**n8q.** n8bn's recipe and data (E2 relabelled, K 102.4, λ 0.5, seed 1, 40 × 100M), plus the relabelled ≤ 8-piece
records as their own sampling group:
- `--min-pieces 2 --groups 2,9 --group-weights 1,9`: exactly 1,638 of every 16,384 records (10%) come from the
  174.8M records with 2–8 pieces;
- the other 14,746 come from the 124.0M records with ≥ 9 pieces in their natural proportions, so within ≥ 9 pieces n8q
  samples exactly as n8bn;
- the ≤ 8-piece results are database-true by construction (E2 relabels each game from its first ≤ 8-piece record);
- export exact (simulated vs engine 1.00); sha256 `681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24`.

**(a) With the database, against n8bn** (0.1 s, 805 pairs, `rtruth.so` fcf6acb):
n8q vs n8bn +1.5 [−2.1, +5.1] (+265 =1087 −258, [0 32 734 39 0]). No difference shown:
the 10% share costs nothing measurable, so n8q5 (5%) was not needed.

**(b) Without the database, against n6c** (the same `.so` in a directory without an `egdb` folder: `get dbpieces`
answers 0; same openings and settings): n8q vs n6c **+2.8 [−1.2, +6.8]** (+289 =1045 −276, [0 39 714 52 0]). It does
not lose. The n8bn-vs-n6c no-database match was moved to the Arch laptop (lead), so how much the blind net loses
there is measured on t3/search-lab.

**(c) Static evals on endings** (no database involved; `scripts/jobs/sanity.sh`): three kings against one, and 400
random 4–6-piece records of gen5a with their database values (side to move):

| position | database (stm) | n6c | n8bn | n8q |
|---|---|---|---|---|
| `B:WK20:BK6,K9,K11` (3 kings vs 1) | win | 517 | 0 | 519 |
| `W:WK20:BK6,K9,K11` (3 kings vs 1) | loss | -392 | -12 | -467 |
| 62 random 4-6-piece database wins | win | mean +236, 98% > +100 | mean +2, 0% > +100 | mean +247, 97% > +100 |
| 277 random 4-6-piece database draws | draw | mean +1, 100% within ±50 | mean -2, 100% within ±50 | mean +1, 99% within ±50 |
| 61 random 4-6-piece database losses | loss | mean -213, 84% < -100 | mean -7, 0% < -100 | mean -238, 92% < -100 |
| `B:WK18,30:B3,12,K19,21` | win | 438 | 2 | 385 |
| `B:WK5,K15:BK7,12,17,K25` | win | 472 | 2 | 376 |
| `W:WK16,K18,20,22:BK10,K32` | win | 405 | 5 | 480 |
| `B:WK6,K8:B15,K18,23,26` | win | 405 | 5 | 355 |
| `B:WK6,K11:B12,K22,23,25` | win | 339 | 3 | 377 |
| `W:W8,K16,K17,32:BK18,K26` | win | 497 | 4 | 433 |
| `B:WK6,K7,16:BK14,19,22` | draw | 3 | -1 | 3 |
| `B:WK1,K6,K7:BK17,K18,K27` | draw | 1 | -3 | 1 |
| `B:WK1,K17:BK23,K32` | draw | 2 | 0 | 2 |
| `B:WK2,13,16:B19,24,K25` | draw | 4 | -2 | 4 |
| `B:WK5,6,K14:BK15,24,K30` | draw | 1 | -1 | 2 |
| `B:WK9,K15,31:BK17,K21,K23` | draw | 7 | -1 | 17 |

n8q is as sane as n6c here and sharper on database losses (92% scored below −100 against 84%). n8bn is blind (≈ 0
everywhere).

**Embedded** (`src/weights/net.bin` = n8q, commit bd1c5c0, not merged until the Book Lab games with the new book are
in):
- ctest **27/27**;
- bench **7,833,556** (default, book on; 8,241,422 with the book off); with n6c it was 7,677,736;
- with n8bn embedded the `search` test fails (26/27).

### P5: wide nets (on n8bn's recipe and E2 data)

- **Nets:** n8bn's recipe (E2, ≥ 9 pieces, K 102.4, λ 0.5, 40 × 100M) with a wider net:
  - w2032 = FT 2048, L1 32;
  - w1064 = FT 1024, L1 64;
  - w2064 = FT 2048, L1 64.
- **Training and export:** after the trainer fix for wide nets (QAT sums in float64; check_export fails above 2
  centi-men simulated-vs-engine). All three exports are exact; simulated vs engine 1.00.
- **Build:** one `rft2048.so` (`REGICIDE_NNUE_MAX_FT=2048`, c0dde2e; engine sources 9b4cfed = master 2efe11a merged)
  for both sides; n8bn through `set evalfile` on the same build.

**Speed:** `bench` with each net through `set evalfile`, 3 runs each under the timing lock + matchlock, median.

| Net | nodes/s | vs n8bn | 0.1 s vs n8bn (805 pairs): W / D / L | Pentanomial | Elo [95% CI] |
|---|---|---|---|---|---|
| n8bn (v1 1024/32) | 3,228,194 | — | | | |
| w1064 (1024/64) | 2,492,847 | −22.8% | 268 / 1070 / 272 | [0 42 725 38 0] | −0.9 [−4.6, +2.9] |
| w2032 (2048/32) | 2,169,969 | −32.8% | 265 / 1073 / 272 | [0 51 710 44 0] | −1.5 [−5.6, +2.6] |
| w2064 (2048/64) | 1,826,125 | −43.4% | 255 / 1071 / 284 | [0 58 718 29 0] | **−6.3 [−10.2, −2.3]** |

**At 0.1 s:**
- w1064 and w2032 show no difference from n8bn (~5–6 Elo resolution).
- w2064 loses: it is 43% slower.
- None pays for its speed at this time control.
- The 0.5 s matches decide, as agreed. They moved to the Arch laptop (lead, 2026-11-10): see
  `experiments/search-lab/RESULTS.md`, "Net matches (for truth-data)", on origin/t3/search-lab, when they exist.

## Phase 3 (2026-11-10 evening)

**E5 forecast** (a forecast, not a measurement; 16:51: 6.61M positions):
- Book Lab's holdout runs take the timing lock but do not create `matchlock`, so under the agreed rules my generators
  keep running during them; only gen6b (the yield group) stops while Book Lab computes (`bookgen`/`lossprobe`).
- With both generators running (~1.9M/h): 15M around 21:30 tonight, 30M around 05:30 on 2026-11-11.
- If gen6b had to yield all night: about 01:30 and 17:30.

**n9h / n9a recipe:** n8q's recipe (E2 files + gen5a ×3, K 102.4, λ 0.5, groups 2–8 and 9–24 with weights 1 : 9) plus
the E5 files gen6a and gen6b, **each given three times** (×3, as gen5a). E5 has no ≤ 8-piece records, so all of its
records fall into the 90% group and are weighted ×3 against the old ≥ 9-piece records there.

### 4a: database-truth on the pilot records (9–12 pieces)

`tools/pilot_truth.py` on the re-run pilot (`data/pilot2_label_rule.csv.gz`): the pilot's records proven at 200k nodes
with the database. These are training records of every net (the held-out version is the database-truth table above).

Pilot records proven at 200k nodes (label.hpp rule): 87370 (wins 45492, losses 41878). Share of proven records whose static eval has the sign of the proven result (eval 0 counts as wrong); in brackets: proven losses scored above -30 + proven wins scored below +30.

| pieces | proven records | n6c | n8bn | n8bn2 | n8c | n8q |
|---|---|---|---|---|---|---|
| 9 | 42654 | 99.93% (83) | 99.94% (65) | 99.94% (56) | 99.95% (57) | 99.94% (65) |
| 10 | 20814 | 99.48% (576) | 99.61% (369) | 99.65% (386) | 99.56% (428) | 99.62% (408) |
| 11 | 13084 | 99.67% (77) | 99.76% (63) | 99.79% (59) | 99.75% (63) | 99.72% (58) |
| 12 | 5839 | 99.31% (211) | 99.38% (129) | 99.42% (124) | 99.35% (132) | 99.40% (138) |
| 9-12 | 82391 | 99.73% (947) | 99.79% (626) | 99.80% (625) | 99.78% (680) | 99.79% (669) |
| 13-14 | 4979 | 99.02% (101) | 99.32% (66) | 99.18% (70) | 99.20% (68) | 99.26% (71) |

**Reading:**
- All nets agree in sign with 99.3–99.95% of the proven positions; 10 and 12 pieces are the weakest counts.
- The truth-data nets (n8bn, n8bn2, n8c, n8q) have about a third fewer weakly scored proofs than n6c (9–12 pieces:
  625–680 against 947), which matches what the E2 labels teach.
- n8q, with its 10% ≤ 8-piece share, is level with n8bn here.

### 4a (addition): draws at 9–12 pieces

The lead asked whether n8q's over-confidence on drawn odd-piece endings (≤ 7 pieces, measured against the database by the reviewer) also shows where the engine has no database. `tools/pilot_draws.py`, no new searches.

Pilot records with 9-12 pieces: 162191; proven at 200k: 82391 (all wins or losses: a search cannot prove a draw above the database); proven draws: 0.

**Draw proxy: not proven at 200k, game's database value a draw** - mean |eval| / share within ±100:

| pieces | material (stm − opp) | records | n6c | n8bn | n8q |
|---|---|---|---|---|---|
| 9 | ±1 | 3115 | 69 / 79% | 86 / 62% | 85 / 62% |
| 9 | all | 3119 | 69 / 79% | 86 / 62% | 85 / 62% |
| 10 | equal | 25054 | 20 / 98% | 22 / 97% | 20 / 98% |
| 10 | all | 25140 | 21 / 98% | 23 / 97% | 21 / 97% |
| 11 | ±1 | 2800 | 74 / 70% | 87 / 62% | 82 / 65% |
| 11 | all | 2806 | 75 / 70% | 88 / 62% | 82 / 64% |
| 12 | equal | 13056 | 28 / 97% | 33 / 93% | 30 / 95% |
| 12 | all | 13152 | 29 / 96% | 34 / 93% | 31 / 94% |

**Stricter draw proxy: as above and |200k score| <= 20** - mean |eval| / share within ±100:

| pieces | material (stm − opp) | records | n6c | n8bn | n8q |
|---|---|---|---|---|---|
| 9 | ±1 | 1854 | 55 / 85% | 61 / 79% | 58 / 80% |
| 9 | all | 1855 | 55 / 85% | 61 / 78% | 59 / 80% |
| 10 | equal | 23057 | 18 / 99% | 19 / 98% | 18 / 99% |
| 10 | all | 23097 | 18 / 99% | 19 / 98% | 18 / 99% |
| 11 | ±1 | 1760 | 52 / 85% | 56 / 81% | 52 / 84% |
| 11 | all | 1760 | 52 / 85% | 56 / 81% | 52 / 84% |
| 12 | equal | 10781 | 22 / 99% | 26 / 97% | 24 / 97% |
| 12 | all | 10825 | 23 / 99% | 26 / 96% | 24 / 97% |

**Proven wins / losses** - share scored beyond ±100 in the proven direction:

| pieces | proven | n6c | n8bn | n8q |
|---|---|---|---|---|
| 9 | 42654 | 92.3% | 98.2% | 98.4% |
| 10 | 20814 | 88.0% | 92.8% | 92.1% |
| 11 | 13084 | 96.5% | 98.4% | 98.3% |
| 12 | 5839 | 85.8% | 91.7% | 90.4% |

**Reading:**
- **There are no proven draws at 9–12 pieces:** a search cannot prove a draw above the database. The draw rows are a proxy: positions the 200k-node search did not prove, from games whose first ≤ 8-piece position is a database draw. They can contain wins the self-play missed; the stricter proxy (200k score within ±20) shows the same pattern, smaller.
- **Yes, at odd piece counts.** Drawn 9- and 11-piece positions are almost all one piece up, and n8bn and n8q score them further from zero than n6c: at 9 pieces mean |eval| 85–86 against 69, and 62% within ±100 against 79%. At 11 pieces 82–88 against 75.
- n8q is slightly closer to zero than n8bn at 11 pieces; at 9 pieces they are the same. Its ≤ 8-piece share does not cause the effect: n8bn, which never saw ≤ 8 pieces, shows it as well.
- **Even counts (mostly equal material) are fine for all three:** 10 pieces 20–23 and 97–98% within ±100; 12 pieces 29–34 and 93–96%.
- **The other side of it:** n8bn and n8q score proven wins and losses beyond ±100 far more often than n6c (9 pieces 98% against 92%, 12 pieces 90–92% against 86%). The database-relabelled targets make the nets more confident about a piece of material: right when it wins, too much when it draws.

### E5: repeated games (review round, 2026-11-10 evening)

**Finding.** A duplicate-game check (`scripts/dupgames.py`: a game = the block from one GAME_START to the next;
duplicate = byte-identical block) found many copies:
- **gen6a:** 34,191 of 149,941 games;
- **gen6b:** 28,546 of 148,139 games;
- 12,515 games in both files.

**Cause and fix:**
- **Not restart replays.** gen6a ran once with seed 611. gen6b's file was fresh at 08:03 (seed 612) and appended once
  with seed 613; the two earlier starts were in deleted files.
- **Cause:** fixed-node play from the same start position replays the same game. 62–70% of the games from 22–24-piece
  random openings (4–12 random plies from the initial position) are copies (up to 117 of one game). Seeded starts (10–17
  pieces) repeat in only 1–8%. The same opening scheme was used for every older generation.
- **Fixes:**
  - datagen now uses each start position once per run;
  - its random streams depend on the records already in the output, so a same-seed restart cannot replay games;
  - it refuses partial-record files;
  - training on E5 uses deduplicated copies (`scripts/dedupe_games.py`: the first copy of each game over all files).
- **Restarts:** both generators were restarted on the fixed binary at 17:42–17:43 (sha256 df0a5f57…, branch ba45aca;
  gen6a seed 614, gen6b seed 615).
  - An old `e5_gen.sh` started by `after_gen4a.sh`, whose script file had been rewritten under it, appended to gen6a for
    about a minute with seed 611. gen6a was cut back to its last verified length (66,707,456 bytes).
- **Measured (18:15):** 9,350,912 records; 6,736,601 left after removing exactly repeated games (72.0%; 258,952 of
  340,920 games). This is the *game-deduplicated record count*, not a count of unique positions.
- **Forecast (not a measurement):** about 2.0M game-deduplicated records per hour; 15M around 22:40, 30M around
  06:10 on 2026-11-11.

**Diversity, measured 2026-11-10 ~19:25** (`scripts/unique_keys.py`; key = board + side to move):

| Data | Records | After removing repeated games | Unique board+side keys |
|---|---|---|---|
| E5 (gen6a + gen6b) | 10,748,672 | 8,565,918 | 7,334,907 (68.2%) |
| gen5a (old, 50k nodes) | 10,435,120 | n/a (no game flags) | 4,892,113 (46.9%) |

- Start positions can still recur across invocations: the per-run check does not see earlier runs. The
  game-deduplicated count removes only byte-identical games.
- Unique keys are the real diversity: E5 has 1.5× gen5a's.

**Validation tails.** For flagged files they now start at a game start. That does not make validation positions unique
against training. Share of validation positions whose key also occurs in the same file's training part:
- gen6a 9.1% (earlier 38.2%, before the repeated starts were fixed and the tail filled with new games);
- gen6b 14.4%;
- gen5a 63.7% (shared openings and transpositions).

Validation losses therefore compare nets with each other, but they are not an independent estimate of generalisation.
These shares are stated again next to n9h's and n9a's validation losses.

**E3 dropped.** The laptop's direct match n8c vs n8bn at 0.5 s, 805 pairs: −1.7 [−5.4, +1.9] (origin/t3/search-lab
d6181de). Re-searched scores add nothing to the database-relabelled results. The rescore of the remaining old files is
dropped for good, and its partial output (gen5c, 135 KB) is deleted.

### 4b: where the lost games were decided

**How to read the loss analysis** (`truth losswalk`, applies to everything below):
- The walk searches each position **without the game's history** and probes the database without the draw rules.
- **"Proven lost"** marks where a 10M-node search with the 8-piece database first proves the loss. That is a reliable
  upper bound on when the game was decided.
- **The "turning move"** is a heuristic: going back from there, the last of Regicide's decisions whose position still
  scored at least −50. It estimates where the game turned. It does not certify the first move that turned a draw into a
  loss.
- For the games that matter most the turn was checked by a large search (below).

#### The five games n8q lost to Cake with book r10 (Book Lab, bl-f4-cake-h1/h2; n6c drew all of them)

**At the first move where n8q leaves n6c's line** (`tools/divergence.py`):
- Both nets' static evals and searches are given, at the node count Regicide used in that game (×1) and ×10.
- Searches: this branch's CLI (master 456e9b2+ engine), `set evalfile`, 8-piece database, fresh hash, no history; a
  move's score is minus the score of the position after it.
- #37 and #75 transpose into the same position.

| ballot (Regicide) | ply (after ballot) | pieces | n8q's move / n6c's move | net | static eval before | static after n8q's move / n6c's move | search ×1 (nodes): n8q's / n6c's move | search ×10: n8q's / n6c's move |
|---|---|---|---|---|---|---|---|---|
| ACF #37 (B) | 15 (12) | 20 | 4-8 / 14-18 | n8q | +0 | -13 / -79 | +0 (d19) / -13 (d19) (1109k) | -5 (d25) / -11 (d24) (11098k) |
| ACF #37 (B) | 15 (12) | 20 | 4-8 / 14-18 | n6c | -34 | -34 / -69 | +0 (d20) / -15 (d18) (1055k) | +0 (d25) / -11 (d23) (10554k) |
| ACF #75 (B) | 15 (12) | 20 | 4-8 / 14-18 | n8q | +0 | -13 / -79 | +0 (d19) / -13 (d19) (1109k) | -5 (d25) / -11 (d24) (11098k) |
| ACF #75 (B) | 15 (12) | 20 | 4-8 / 14-18 | n6c | -34 | -34 / -69 | +0 (d20) / -15 (d18) (1055k) | +0 (d25) / -11 (d23) (10554k) |
| ACF #87 (B) | 13 (10) | 22 | 1-6 / 8-11 | n8q | -76 | -62 / -54 | -83 (d20) / -41 (d18) (1295k) | -88 (d25) / -59 (d23) (12955k) |
| ACF #87 (B) | 13 (10) | 22 | 1-6 / 8-11 | n6c | -67 | -83 / -65 | -27 (d20) / -23 (d19) (2259k) | -51 (d26) / -51 (d24) (22597k) |
| ACF #124 (B) | 11 (8) | 22 | 15-19 / 8-12 | n8q | -20 | -106 / -35 | -18 (d19) / +0 (d18) (1133k) | -32 (d25) / +0 (d25) (11331k) |
| ACF #124 (B) | 11 (8) | 22 | 15-19 / 8-12 | n6c | -25 | -67 / -28 | -28 (d19) / -2 (d18) (893k) | -18 (d23) / +0 (d24) (8934k) |
| ACF #131 (W) | 6 (3) | 22 | 21-17 / 28-24 | n8q | +7 | +10 / -7 | +2 (d19) / +11 (d18) (1321k) | -8 (d24) / +6 (d23) (13218k) |
| ACF #131 (W) | 6 (3) | 22 | 21-17 / 28-24 | n6c | +2 | +8 / -2 | +12 (d18) / +23 (d17) (811k) | +0 (d23) / +9 (d22) (8116k) |

- **No misjudgement at the divergence.** Neither net judges either candidate lost. n8q's move costs 10–40 centi-men
  against n6c's move in three of the four distinct positions (#87, #124, #131), and in #37/#75 it is the slightly
  better one.
- Re-searched without the game's history and hash, n8q's own search even prefers n6c's move in #124 and #131. So the
  in-game choice was close and depended on the search state, not on a clear evaluation difference.

**Where they were lost** (losswalk, 10M nodes per position; turning moves confirmed by 100M-node searches before and
after the move, with either net's evaluation):

| game (Regicide) | turn: pieces | move played | net | search before (Regicide to move) | best move | move played: score after (Regicide's view) |
|---|---|---|---|---|---|---|
| 9-14 22-17 6-9 (black) | 17 | 5x14 | n8q | -47 (d33) | 5x14 | -33 (d33) |
| 9-14 22-17 6-9 (black) | 17 | 5x14 | n6c | -31 (d32) | 5x14 | -29 (d33) |
| 10-15 21-17 9-13 (black) | 22 | 6-9 | n8q | -82 (d30) | 6-9 | -23 (d30) |
| 10-15 21-17 9-13 (black) | 22 | 6-9 | n6c | -32 (d29) | 6-9 | -21 (d30) |
| 11-15 23-18 12-16 (black) | 18 | 10-15 | n8q | +0 (d30) | 8-12 | -167 (d32) |
| 11-15 23-18 12-16 (black) | 18 | 10-15 | n6c | +0 (d31) | 8-12 | -107 (d31) |
| 11-15 24-20 12-16 (white) | 17 | 23-19 | n8q | -83 (d34) | 23-19 | -74 (d35) |
| 11-15 24-20 12-16 (white) | 17 | 23-19 | n6c | -54 (d34) | 23-19 | -57 (d34) |

**Reading:**
- **Proven lost:** #37/#75 at 12 pieces (move 21), #87 at 12 (move 37), #124 at 11 (move 25), #131 at 10 (move 39).
- **One clear error, at 18 pieces (#124).** The position before is level (0 with either evaluation); 10-15 drops it to
  −167 (n6c's evaluation: −107), while n8q's 0.5 s search had scored the move −8. That is a search-horizon miss; a deep
  search with either net finds it.
- **In the other games the estimated turning move was the best move** of the 100M-node search. The position was already
  somewhat worse (−30 to −80 at 17–22 pieces): those games slid from the opening phase, starting with the cheaper
  choices at the divergence (plies 6–15, 20–22 pieces).
- **This matches the reviewer's measurement:** one step beyond the database the search is nearly error-free. These
  losses are made early, with 17–22 pieces, by small concessions and once by a horizon miss, not at the database boundary.
- **Would E5-style data help here?** It covers exactly this region: seeds from plies 6–30 of real games against Cake,
  KingsRow and GuiNN, and 24% of its records have 17–24 pieces. But the errors are 10–40 centi-men choices between
  playable moves (one horizon miss aside), the kind of difference that lies within the noise of a net's evaluation. Better
  labels there may help a little; a verdict needs n9h/n9a against these ballots, not only self-play.

#### All book-off lost games against Cake and GuiNN (all 25 runs, final 2026-11-11 01:10)

Per-game rows: `logs/lossreport_final.md`; raw output: `logs/losswalk/lw_*.tsv`. Read with the caveats at the top of
4b: no game history, and the turn is a heuristic. KingsRow games are not part of it (see the k3 section below).
- The n4c rows are the older built-in net's book-off runs (`ba-off-769*`, `ba-tm-off`).
- "in-game score 99999" in the rows marks a move without an in-game score.

**Summary by opponent and net** (horizon column corrected 2026-11-11, review r17)

**Correction:** the first version of this table treated an in-game score of exactly 0 as missing, so 9 rows were left
out of the horizon count. The affected cells changed: Cake/n4c 3 → 5, Cake/n6c 6 → 9, Cake/n8bn 3 → 6, GuiNN/n6c 1 → 2.
What this changes in the conclusion: more of the big drops were invisible to the in-game search than first reported,
25 of 31 rows instead of 16.

Turning moves whose deep score drops by 100 or more are classed as:
- 'horizon' if Regicide's in-game score for the move stayed above −50;
- 'eval' if the net's static eval of the position after the move is ≥ 0.

| opponent | net | lost games | proven lost: median pieces | already ≤ −50 at Regicide's first decision | turn: games at 20–24 / 17–19 / 13–16 / 9–12 / ≤ 8 pieces (others) | drops ≥ 100 at the turn | of which horizon / eval |
|---|---|---|---|---|---|---|---|
| Cake | n4c | 41 | 9 | 0 | 25 / 10 / 4 / 2 / 0 | 6 | 5 / 2 |
| Cake | n6c | 62 | 11.0 | 0 | 30 / 14 / 15 / 3 / 0 | 10 | 9 / 0 |
| Cake | n8bn | 15 | 12 | 0 | 8 / 6 / 1 / 0 / 0 | 8 | 6 / 0 |
| Cake | n8c | 5 | 11 | 0 | 5 / 0 / 0 / 0 / 0 | 1 | 1 / 0 |
| Cake | n8q | 5 | 12 | 0 | 1 / 4 / 0 / 0 / 0 | 1 | 1 / 0 |
| GuiNN | n4c | 5 | 9 | 0 | 3 / 1 / 1 / 0 / 0 | 0 | 0 / 0 |
| GuiNN | n6c | 8 | 9.0 | 0 | 6 / 0 / 2 / 0 / 0 | 4 | 2 / 0 |
| GuiNN | n8bn | 2 | 9.5 | 0 | 1 / 0 / 1 / 0 / 0 | 1 | 1 / 0 |
| GuiNN | n8c | 2 | 12.0 | 0 | 2 / 0 / 0 / 0 / 0 | 0 | 0 / 0 |

**All 145 analysed games:**
- Proven lost at a median of 10 pieces.
- Turns at 20–24 / 17–19 / 13–16 / 9–12 / ≤ 8 pieces: 81 / 35 / 24 / 5 / 0.
- Unresolved or not proven: 0. Already ≤ −50 at Regicide's first decision: 0.

**Repeated games.** Many of the 145 are the same game repeated across runs: the engines are deterministic with the book
off. For example, ballot "11-16 23-19 16x23" as White is lost the same way in 9 runs. Counted once per (opponent, ballot,
colour, turn, deep scores), about 73 distinct games remain. Their turns fall at 20–24 / 17–19 / 13–16 / 9–12 / ≤ 8
pieces in 39 / 16 / 14 / 4 / 0 games; they are proven lost at a median of 10 pieces; 17 have a drop ≥ 100 at the turn.

**Reading:**
- The games are proven lost at a median of 10 pieces, but the deep score falls below −50 much earlier: in most games
  while 20–24 pieces are still on the board, and in none at ≤ 8 pieces. That agrees with the reviewer's measurement that
  the search is nearly error-free one step beyond the database.
- The losses are made in the opening and early middle game, and the picture is the same for every net (n4c to n8q).
- Most turns are small drops (−20 … −60 cm) that only become visible later. Large drops at the turn are few: 17 of
  about 73 distinct games. In 25 of the 31 rows the in-game search did not see the drop ('horizon'), and the net's static
  eval was positive after the move only for n4c (2 games).
- This matches the KingsRow k3 lines below: search depth at 14–24 pieces, not the endgame, decides these games.


#### KingsRow, book mode 3: the seven games Regicide lost as Black (Book Lab k3; lead, 2026-11-10)

The seven games (origin/t3/booklab `experiments/booklab/k3/`, `k3_losses.md`) are four distinct lines:
- #60: a-h1 and b-h1 52.1, the same game.
- #71: a-h1 and b-h1 63.1, the same game.
- #141/#9: #141 a-h2 49.1 and #9 b-h1 8.1. The ballots differ, but the games are the same from ply 4 on by
  transposition.
- #34: b-h1 31.1.

For every position Regicide (Black) had to move, from the end of the ballot to the first position a 20M-node search
(n6c, 8-piece database) proves lost, we record:
- the static eval and a 1M-node search of n6c and of n8q;
- the exact positions and their one-move neighbours in the training data: the E2 files gen1–gen5c with their original
  results, and the E5 files gen6a/gen6b.

Searches: this branch's CLI, book off, fresh hash, game history kept. Ply k = the position after k plies (Black to
move); `k3_losses.md` counts the move made there as ply k+1, and its plies are converted to this numbering in the table.
"Turn" = Black's moves after which the 20M value fell by ≥ 40 cm.

| line | Book Lab (in game): own search from / own score ≤ −100 from | turn: ply (pieces), 20M before → after | game move; 20M, n6c 1M, n8q 1M choose | there: static n6c / n8q; 1M n6c / n8q | proven lost (20M): ply, pieces | in training (exact) |
|---|---|---|---|---|---|---|
| #60 | 12 / 18 | 12 (18): −39 → −81; 18 (18): −109 → −177 | 2-6; 3-7, 2-6, 3-7 — 16-20; 5-9, 5-9, 16-20 | ply 18: −71 / −82; −96 / −104 | 102, 9 (20M at −100 to −130 from ply 26 to 100) | to ply 12 (ply 6/10: ~750 records each, mostly D/L, mean −34; ply 12: 1, L); later only ply 48 (10 pieces: 5, L, −126) |
| #71 | 4 / 16 | 12 (20): −21 → −88; 14 (20): −88 → −129 | 2-7; 3-7, 3-7, 3-7 — 7-10; 5-9, 5-9, 7-11 | ply 14: −39 / −44; −57 / −28 | 44, 11 | to ply 14 (ply 8: 607, mostly D; ply 12: 6, D; ply 14: 1, L) |
| #141/#9 | 10 / 48 | slide under 20M's own moves: 36 (17) −35, 38 −75, 40 −118, 42 −226, 44 −322, 48 −511, 52 −724 | 20M agrees with the game at 36, 38, 44, 46, 50, 52; prefers 12-16, 11-16, 13-17 at 40, 42, 48 | plies 40–52: n6c −16 … −101 (one −238 before a capture), n8q −10 … −243; 1M n6c −28 … −129, n8q −82 … −405 | 54, 15 | to ply 18 (18 pieces; ply 6: 1129 records, D/L, mean −27); none from ply 20 on |
| #34 | 12 / 16 | 26 (14): −58 → −122 | 11-15; 10-15, 11-15, 10-15 | ply 26: −40 / −32; −67 / −128 | 46, 11 | to ply 26 (14 pieces): plies 12–26 each 72 records, all from one game repeated 36 times in gen5a and in gen5c, labelled lost (E2 and original), score −30 … −71 |

One-move neighbours add nothing beyond the plies with exact hits: after ply 20 there are only two single records.

Files:
- `data/k3/k3_lines.md`: full per-ply tables.
- `data/k3/k3_hits.txt`: training hits per file.
- `data/k3/k3_decisions.txt`: move choices at each drop.
- `scripts/k3.py`: the script.

**Reading.**
- **Three of the four lines (#60, #71, #34) turned on 40–70 cm choices at 14–20 pieces.**
  - At each turn, the 20M search prefers a move other than the game's.
  - The two nets' 1M searches split. n6c picks the game's move at #60 ply 12 and at #34. n8q picks it at #60 ply 18.
    At #71 ply 12, all three searches pick 3-7, but the game's own search played 2-7.
  - At #60 ply 12 the losing move was the game's first own search after the book.
  - The static evals at the turns are only mildly negative (−30 … −80). A 1M search with either net reaches the 20M
    verdict within one move:

    | position | 1M n6c / n8q | 20M |
    |---|---|---|
    | #60 ply 20 | −124 / −137 | −177 |
    | #71 ply 16 | −109 / −190 | −129 |
    | #34 ply 30 | −119 / −175 | −148 |

  - **This is search depth, not a structure the net cannot judge.** These positions are not proven: the 20M value is
    n6c's own opinion. It is not checked that 20M's alternative move actually holds the draw.
- **#141/#9 is different.**
  - The 20M value slides from −35 to −724 over eight Black moves, while 20M agrees with most of the game's moves. So the
    game was already worse at ply 36 (17 pieces) than even 20M saw: the error lies there or earlier, beyond its horizon.
  - At plies 40–52, where 20M says −118 … −724, n6c's static eval and its 1M search stay at −20 … −130. **n6c
    misjudges this structure, and only the 20M search makes up for it.**
  - n8q's 1M search is closer (−82 … −405) but erratic.
  - The game's own search fell to ≤ −100 only at ply 48, eight plies after 20M did.
- **Training data.**
  - From the turn onwards, no position of any line is in the data, and neither is any one-move neighbour, except in #34.
  - In #34, the line up to and including the turn position (14 pieces) is in gen5a/gen5c 72 times. All 72 records come
    from one seeded game, repeated 36 times in each file. All are labelled lost, with mild scores (−30 … −71), and the
    nets' mild static evals there match those scores.
  - All other exact hits are opening positions (18–24 pieces), with mixed draw/loss labels.
  - The 8-piece database plays no part: the proofs come at 9–15 pieces, long after the turns.
- **Caveats.**
  - There is one game per line.
  - The 20M reference search uses n6c, so it is not independent of n6c.
  - The game's own search disagrees with 20M at times: at #34 it was at ≤ −100 from ply 16, where 20M says −25.
  - KingsRow-based results stay provisional (see the book-mode note in P1).

<!-- PHASE3MORE -->

## What holds and what does not

Each claim is read at the resolution of its test (see "Reading the evidence").

- **Wrong targets: yes, for the game result.**
  - 405,095 games end with a result the database contradicts: 14.0% of all 2,894,895 inferred games, or 15.2% of the
    2,666,528 that reach ≤ 8 pieces (18–24% of those in gen1–gen3a).
  - Almost always the database says win and the self-play drew.
  - On proven 9–14-piece positions the stored result is wrong 18.5% of the time in the 10k-node files (17.5% over all
    files).
  - Replacing it with the database value (E2) is the step that gains: n8bn +4.5 [+0.5, +8.6] at 0.1 s and
    +4.5 [+1.6, +7.4] at 0.5 s/move (one seed, the best of 11 recipes). The seed-2 replicate n8bn2 gives
    +3.0 [−0.9, +7.0]; both seeds pooled +3.8 [+0.9, +6.6].
- **Wrong positions: no effect shown by themselves.**
  - Dropping the ≤ 8-piece records (58.5% of the data) shows no difference at n6c's K (n8ak +0.9 [−2.7, +4.4]).
  - **But a net trained without them is blind there:** n8bn evaluates three kings against one at 0, and with n8bn as
    the built-in net the engine's search test fails.
  - Engines run without the database need those positions, so the built-in candidate n8q keeps them as 10% of every
    batch. That shows no cost with the database (+1.5 [−2.1, +5.1] vs n8bn) and keeps the endings right (T4).
- **The stored search scores were not wrong in sign.**
  - On positions the database search proves, the stored score had the wrong sign 17 times in 88k; its size was
    moderate (median +236).
  - E3 (re-searched, proven-exact scores) shows no gain over E2: n8c +0.4 against n8bn's +4.5 vs n6c. That comparison
    resolves only ~8 Elo; the direct match is on the Arch laptop's list.
- **K matters for the eval scale.**
  - Database-true results correlate so well with the scores that the refit K drops to 43 (from 102). That shrinks the
    eval: n8b scores +1.1 against n8bn's +4.5 at K 102.4.
  - Between K 77, 102 and 130 no difference is shown (~8 Elo). Truth-labelled nets should keep n6c's K or tune it
    separately.
- **The database-truth metric hardly separates nets.**
  - Every net agrees in sign with 99.7–99.8% of the held-out proven 9–12-piece positions; proven positions are mostly
    lopsided.
  - The E2/E3 nets at a large K do best on the tails, in line with the matches, but the metric would not have
    predicted the match results on its own.
  - Per-group validation loss does not rank nets of one family either.
- **Against the opponents**, with the book off, no net difference is shown at ~6–17 Elo:
  - the conversion search (master 2efe11a) is what turned GuiNN games into wins (+16 to +18 for n6c, n8bn and n8c);
  - Cake still wins 5–7 of 312 games against each net.

## All matches

Every match of this experiment, with the engine build both sides used (self-play) or Regicide used (against an
opponent), and the commit that build was made from. Generated from the match directories (`scripts/all_matches.py`).
Each comparison uses a single build.

| match | engine (both sides, or Regicide vs an opponent) | built from | time | games | A: W / D / L | pentanomial | Elo [95% CI] |
|---|---|---|---|---|---|---|---|
| n6d-vs-n6c | rff0a3d2.so | ff0a3d2 | 0.1s | 1610 | +267 =1066 -277 | [0 42 731 32 0] | -2.2 [-5.8, +1.5] |
| n7a-vs-n6c | rff0a3d2.so | ff0a3d2 | 0.1s | 1610 | +269 =1071 -270 | [0 45 716 44 0] | -0.2 [-4.2, +3.8] |
| n7b-vs-n6c | rff0a3d2.so | ff0a3d2 | 0.1s | 1610 | +263 =1072 -275 | [0 50 717 38 0] | -2.6 [-6.6, +1.4] |
| n8a-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +264 =1067 -279 | [0 52 716 37 0] | -3.2 [-7.2, +0.7] |
| n8ak-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +268 =1078 -264 | [0 33 735 37 0] | +0.9 [-2.7, +4.4] |
| n8b-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +273 =1069 -268 | [0 36 728 41 0] | +1.1 [-2.6, +4.8] |
| n8bk-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +283 =1060 -267 | [0 42 705 58 0] | +3.5 [-0.8, +7.7] |
| n8bk130-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +273 =1079 -258 | [0 34 722 49 0] | +3.2 [-0.6, +7.1] |
| n8bl6-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +268 =1073 -269 | [0 43 720 42 0] | -0.2 [-4.1, +3.7] |
| n8bl7-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +278 =1068 -264 | [0 35 721 49 0] | +3.0 [-0.8, +6.9] |
| n8bn-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +276 =1079 -255 | [0 36 712 57 0] | +4.5 [+0.5, +8.6] |
| n8bn-vs-n6c-0.5-a | rtruth.so | fcf6acb | 0.5s | 414 | +65 =290 -59 | [0 1 199 7 0] | +5.0 [+0.4, +9.6] |
| n8bn-vs-n6c-0.5-b | rtruth.so | fcf6acb | 0.5s | 410 | +82 =252 -76 | [0 5 189 11 0] | +5.1 [-1.5, +11.7] |
| n8bn-vs-n6c-0.5-c | rtruth.so | fcf6acb | 0.5s | 394 | +52 =294 -48 | [0 4 185 8 0] | +3.5 [-2.4, +9.5] |
| n8bn-vs-n6c-0.5-d | rtruth.so | fcf6acb | 0.5s | 392 | +68 =261 -63 | [0 3 185 8 0] | +4.4 [-1.3, +10.2] |
| n8bn2-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +280 =1064 -266 | [0 37 717 51 0] | +3.0 [-0.9, +7.0] |
| n8c-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +263 =1086 -261 | [0 38 727 40 0] | +0.4 [-3.3, +4.2] |
| n8p-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +266 =1075 -269 | [0 39 730 36 0] | -0.6 [-4.3, +3.0] |
| n8p-vs-n8bn | rtruth.so | fcf6acb | 0.1s | 1610 | +264 =1084 -262 | [0 45 713 47 0] | +0.4 [-3.6, +4.5] |
| n8p2-vs-n6c | rtruth.so | fcf6acb | 0.1s | 1610 | +278 =1072 -260 | [0 32 723 50 0] | +3.9 [+0.1, +7.7] |
| n8p2-vs-n8bn | rtruth.so | fcf6acb | 0.1s | 1610 | +264 =1080 -266 | [0 40 727 38 0] | -0.4 [-4.2, +3.3] |
| n8q-vs-n6c-nodb | rtruth.so without an egdb folder | fcf6acb | 0.1s | 1610 | +289 =1045 -276 | [0 39 714 52 0] | +2.8 [-1.2, +6.8] |
| n8q-vs-n8bn | rtruth.so | fcf6acb | 0.1s | 1610 | +265 =1087 -258 | [0 32 734 39 0] | +1.5 [-2.1, +5.1] |
| ref-ponder-pt-s2-a1-cake | DLL ponder-df7780a | df7780a (ponder agent's build and run) | 0.5s | 312 | +0 =307 -5 | [0 5 151 0 0] | -5.6 [-10.4, -0.8] |
| ref-ponder-pt-s2-a2-cake | DLL ponder-df7780a | df7780a (ponder agent's build and run) | 0.5s | 312 | +0 =307 -5 | [0 5 151 0 0] | -5.6 [-10.4, -0.8] |
| tr-n6c-cake-c0dde2e | DLL truth-c0dde2e | c0dde2e (engine sources 9b4cfed: master 2efe11a merged) | 0.5s | 312 | +0 =305 -7 | [0 7 149 0 0] | -7.8 [-13.4, -2.2] |
| tr-n6c-guinn-c0dde2e | DLL truth-c0dde2e | c0dde2e (engine sources 9b4cfed: master 2efe11a merged) | 0.5s | 312 | +19 =290 -3 | [0 3 135 17 1] | +17.8 [+7.5, +28.2] |
| tr-n6c-kingsrow | DLL ponder-df7780a | df7780a (ponder agent's build) | 0.5s | 312 | +1 =309 -2 | [0 2 153 1 0] | -1.1 [-4.9, +2.7] |
| tr-n8bn-cake-c0dde2e | DLL truth-c0dde2e | c0dde2e (engine sources 9b4cfed: master 2efe11a merged) | 0.5s | 312 | +0 =307 -5 | [0 5 151 0 0] | -5.6 [-10.4, -0.8] |
| tr-n8bn-cake1 | DLL ponder-df7780a | df7780a (ponder agent's build) | 0.5s | 312 | +0 =307 -5 | [0 5 151 0 0] | -5.6 [-10.4, -0.8] |
| tr-n8bn-cake2 | DLL ponder-df7780a | df7780a (ponder agent's build) | 0.5s | 312 | +0 =307 -5 | [0 5 151 0 0] | -5.6 [-10.4, -0.8] |
| tr-n8bn-guinn-c0dde2e | DLL truth-c0dde2e | c0dde2e (engine sources 9b4cfed: master 2efe11a merged) | 0.5s | 312 | +16 =294 -2 | [0 2 138 16 0] | +15.6 [+6.7, +24.6] |
| tr-n8bn-kingsrow | DLL ponder-df7780a | df7780a (ponder agent's build) | 0.5s | 312 | +1 =311 -0 | [0 0 155 1 0] | +1.1 [-1.1, +3.3] |
| tr-n8c-cake-c0dde2e | DLL truth-c0dde2e | c0dde2e (engine sources 9b4cfed: master 2efe11a merged) | 0.5s | 312 | +0 =307 -5 | [0 5 151 0 0] | -5.6 [-10.4, -0.8] |
| tr-n8c-guinn-c0dde2e | DLL truth-c0dde2e | c0dde2e (engine sources 9b4cfed: master 2efe11a merged) | 0.5s | 312 | +17 =293 -2 | [0 2 137 17 0] | +16.7 [+7.6, +25.9] |
| w1064-vs-n8bn | rft2048.so (REGICIDE_NNUE_MAX_FT=2048) | c0dde2e (engine sources 9b4cfed) | 0.1s | 1610 | +268 =1070 -272 | [0 42 725 38 0] | -0.9 [-4.6, +2.9] |
| w2032-vs-n8bn | rft2048.so (REGICIDE_NNUE_MAX_FT=2048) | c0dde2e (engine sources 9b4cfed) | 0.1s | 1610 | +265 =1073 -272 | [0 51 710 44 0] | -1.5 [-5.6, +2.6] |
| w2064-vs-n8bn | rft2048.so (REGICIDE_NNUE_MAX_FT=2048) | c0dde2e (engine sources 9b4cfed) | 0.1s | 1610 | +255 =1071 -284 | [0 58 718 29 0] | -6.3 [-10.2, -2.3] |

## Running in the background, and how to resume

**Where things are.** All job scripts are in `C:\regicide-match\jobs\truth`, with copies in `scripts/jobs/`. Status
lines go to `~/regicide-jobs/truth/status` and `~/regicide-jobs/r4/status` (prefix `TR`). `launch.sh SCRIPT ARGS`
starts a script detached. Scripts that play CheckerBoard DLLs (`run_cb.sh`, `p1*.sh`) need an attached session:
`Start-Process -WindowStyle Hidden wsl.exe -ArgumentList 'bash','/mnt/c/regicide-match/jobs/truth/<script>'`.

**Lock rule (lead, 2026-11-10):** every hold of `~/regicide-jobs/locks/timing.lock` stays under 15 minutes. Longer
matches are split into chunks (as `confirm.sh` does).

- **E5 datagen (P4):** two processes, each appending to its own file.
  - `gen6a_n50k_egdb.bin`: `datagen-tr6`, 6 base-budget threads, seed 611.
  - `gen6b_n50k_egdb.bin`: `datagen-tr5`, 6 extra threads that yield. Seed 612, then 613 after the pilot re-run (a new
    seed avoids repeating games).
  - Goal: 30M positions together. The finishing time is a forecast, given in P4.
  - Stop: `pkill -x datagen-tr5; pkill -x datagen-tr6` (each file keeps every finished game).
  - Restart: `launch.sh e5_gen.sh NAME BIN SEED THREADS` with a new seed. datagen appends only to files that carry
    game-start flags.
- **Pilot re-run:** `pilot2.sh` (6 threads from gen6b, which it restarts when done) → `~/regicide-jobs/truth/pilot2.csv`.
- **Rescore of the remaining files:** parked after gen4a (gen5c, gen3d … gen1 remain, about 29 h of 8 threads).
  - Resume: `bash rs_threads.sh 6`; `rescore_chain.sh` continues every file where it stopped.
  - It uses the partial-loss rule (`rescore-tr4`).
- **Moved to the Arch laptop (lead, 2026-11-10):** P5's 0.5 s matches, n8bn vs n6c without the database, and n8c vs
  n8bn. Results go to origin/t3/search-lab, `experiments/search-lab/RESULTS.md`, "Net matches (for truth-data)".
- **Keepers:**
  - `pausekeeper_truth.sh` runs while `~/regicide-jobs/locks/pk_hold` exists. Under `r4/matchlock` it stops
    `^truth-tr`, `^rescore-tr` and `^datagen-tr` (`locks/bg_patterns`).
  - The yield group (`locks/bg_patterns_yield`: `^datagen-tr5`) also stops under `locks/yield_truth` (my CheckerBoard
    runs) or while a `bookgen-*`/`lossprobe*` process runs.
  - Book Lab's `pausekeeper5.sh` is unchanged.
- **Nothing of mine holds** `timing.lock` or `matchlock` between runs.

## Files

- `scripts/`: `census_report.py`, `pilot_report.py`, `db_truth.py`; `jobs/` (the shell jobs as run).
- Logs and tables: `logs/`.
- Data on the Windows box only: `~/regicide-data/truth/` (`*_e2.bin` relabelled files, `*_e3.bin` rescored files,
  `tails/` the held-out proven set, `seeds_real_6_40.txt`).
