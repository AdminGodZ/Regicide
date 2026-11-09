# book-deep2: second, deeper pass on the ballots Cake still wins against the deep book (master 6bb8c6c)

**Status (2026-11-09, pushed before the 22:00 presentation).** The candidate is **book_deep2b** (section 3).
- **book_deep2** (150M-node pass on 5 ballots + 6 lossprobe avoid entries) is **not better overall** than the master
  book (book_deep) on those ballots: **+13.9 Elo [−7.3, +35.1]** over 300 + 300 targeted games against Cake (LOS 90%),
  **not significant**.
- **Per ballot, it is nearly deterministic and mixed:**
  - fixes White Doctor (B, 20/30 → 0/30 losses) and Wilderness II (B, 8/30 → 1/30);
  - improves #7 (B, 30/30 → 22/30);
  - leaves Black Widow (W) unchanged (28/30 → 28/30);
  - **breaks Double Cross (W, 6/30 → 30/30)**: book2's deeper analysis switches Regicide's 2nd move from 18-14 to
    26-23, and Cake wins every game after 26-23.
- **book_deep2b** keeps book_deep's Double Cross subtree and uses book2 only for the other 4 ballots, with the same 6
  avoid entries. **On the 5 targeted ballots against Cake it is +59.9 Elo [+46.8, +73.0] over book_deep** (600 vs 800
  games):
  - Regicide lost 98 of 600 games against 260 of 800;
  - it fixes White Doctor, Wilderness II and Double Cross completely (0 losses in 60 each) and halves the #7 losses.
  - **It is the install candidate.** The full acf156 Cake and KingsRow runs are still needed (not run, for the
    deadline).
- **Skipped for the deadline** (setups ready, see "Next steps"): the full acf156 Cake and KingsRow runs with the new
  book, the round-7 matches, and book_deep3 (merge with Arch's acf156 book).

## 1. What was built

- **Engine and tools:** master **6bb8c6c** (built-in n6c + the deep book `book_deep.bin`, bench 7,712,020; 8,089,571
  with `set book off`; ctest 13/13; `logs/build_6bb8c6c.txt`). `bookgen` and `lossprobe` come from the same tree.
- **Ballots:** the 5 that Cake won at least twice with book_deep across the three book-on runs (`results` on
  `t3/book-deep`). No other ballot qualified.
  - ACF #7 (9-13 22-18 10-15) as Black: lost 3/3 runs;
  - ACF #153 Black Widow (11-16 23-19 16x23) as White: 3/3;
  - ACF #46 Double Cross (9-14 23-18 14x23) as White: 2/3;
  - ACF #68 White Doctor (10-14 22-18 12-16) as Black: 2/3;
  - ACF #16 Wilderness II (9-13 23-18 11-16) as Black: 2/3.
  - Roots: `roots2.txt`, the position after each ballot, White to move. Ballot list: `ballots2.txt`.
- **lossprobe** on the 12 book-on Cake losses (`losses_bookon.pdn`; `report2.txt`):
  - `--movetime 30000 --threads 6 --jobs 2 --hash 1024`, 2–8 EGDB.
  - Strict thresholds so that only confident blames count: `--drop 50 --margin 40` (defaults 30/20).
  - **8 blamed, 4 unclear** (all 3 Black Widow losses and one #7). That gives **6 avoid entries** (`avoid2.bin`):

| Ballot | Ply | Move | Drop / margin (cp) | Times |
|---|---|---|---|---|
| #7 | 58 | 23-26 | 251 / 219 | 1 |
| #7 | 84 | 30-25 | 63 / 49 | 1 |
| Wilderness II | 10 | 16-19 | 65 / 47 | 1 |
| Wilderness II | 36 | 9-13 | 98 / 93 | 1 |
| Double Cross | 19 | 24-20 | 69 / 65 and 125 / 121 | 2 |
| White Doctor | 12 | 9-14 | 78 / 85 and 96 / 105 | 2 |

- **book2:** `bookgen --roots roots2.txt --nodes 150000000 --threads 20`.
  - **150M nodes per analysed move**, 5× book_deep's 30M.
  - **Calibration** (13:40–14:22, `logs/calib.log`): 5 roots in 366 s, then 106 expansions in 2,154 s
    (0.049/s).
  - **Main run** (`--resume`, `--expansions 673`, 14:22–18:37, `logs/main.log`): about 0.04/s. In total **720
    expansions, 831 positions**.
  - Root values ended between −52 and −21 for the side to move (White), e.g. the first root at −52.
- **Merge** (`scripts/merge_books.py`): book_deep (5,646 positions, 32,541 entries) + book2 (831 positions, 4,950
  entries).
  - 523 positions are in both; there the deeper analysis wins per (position, move), which was always book2's.
  - The merge was re-saved through `bookgen --resume --roots roots.txt (all 46) --avoid avoid2.bin --expansions 0`.
  - Result: **book_deep2.bin, 5,954 positions + 6 avoid entries** (`logs/merge.log`, `logs/finalize.log`). No book
    position was dropped by the avoid list.
- **DLL** `6bb8c6c-deep2-egdb8`: master 6bb8c6c with `book_deep2.bin` embedded, 2–8 EGDB (`logs/build_deep2.txt`).
  - bench 7,712,020 with the book on (it differs only inside the 5 subtrees) and 8,089,571 with `set book off`;
  - `dbpieces` 8, book mode 3, built-in avoid list 6 entries.

## 2. Targeted test: book_deep2 vs book_deep against Cake

- **Openings:** `target_openings.txt` lists the 5 ballots 10 times each. Each run plays them with both colours: 100
  games, 0.5 s/move, `-c 8`, hash 128, 2–8 EGDB, Cake as in the baselines.
- **Runs:** interleaved, book_deep2 (`tests/d2t*`) then the master DLL `6bb8c6c-egdb8` (`tests/d1t*`), 18:41–19:29.
- **Background:** nothing (gen5c had finished, gen4b was paused).

| Run | W / D / L | Elo [95% CI] |
|---|---|---|
| d2t1 / d1t1 | 0/73/27 vs 0/69/31 | −96.2 vs −111.4 |
| d2t2 / d1t2 | 0/72/28 vs 0/67/33 | −100.0 vs −119.1 |
| d2t3 / d1t3 | 0/74/26 vs 0/72/28 | −92.5 vs −100.0 |
| **pooled (300 each)** | **0/219/81 vs 0/208/92** | **−96.2 [−111.2, −81.2] vs −110.1 [−125.1, −95.1]** |

**book_deep2 − book_deep: +13.9 Elo [−7.3, +35.1], LOS 90.1%. Not significant.** (`pool_targeted.txt`)

**Per ballot and colour** (`per_ballot.txt`, Regicide's losses / games):

| Ballot | Regicide as | book_deep | book_deep2 |
|---|---|---|---|
| ACF #7 | B | 30 / 30 | **22 / 30** |
| Wilderness II | B | 8 / 30 | **1 / 30** |
| White Doctor | B | 20 / 30 | **0 / 30** |
| Black Widow | W | 28 / 30 | 28 / 30 |
| **Double Cross** | W | 6 / 30 | **30 / 30** |
| (the other colour of each ballot) | | 0 | 0 |

- **These ballots are very hard against Cake.** Both books lose about a third of the games on them. Black Widow as
  White is lost about 93% of the time either way, and lossprobe found no confident blame there.
- **The results are close to deterministic per ballot and colour:** the same lines are repeated, so the 300 games are
  far from independent. The CI above understates the uncertainty in one sense, since the effective sample is ten
  ballot-colour cases. It overstates it in another, since each case is nearly fixed. **The per-ballot table is the more
  useful result.**
- **Double Cross:**
  - With book_deep, Regicide (White) plays 27x18 12-16 **18-14**: 24 draws, 6 losses.
  - With book_deep2 it plays **26-23**, book2's 150M-node choice: 30 losses out of 30.
  - The avoid entry for this ballot (24-20 at ply 19) is not involved. It lies in the 18-14 line.

## 3. book_deep2b: book2 without the Double Cross root

- **Built at 19:31** (`scripts/d2b.sh`):
  - `book2b`: `bookgen --resume book2.bin` with the 4 other roots (`logs/roots2b.txt`), `--expansions 0`. 746
    positions.
  - Merged into book_deep (445 positions overlapping) and re-saved with the same 6 avoid entries:
    **`book_deep2b.bin`, 5,947 positions + 6 avoid entries**.
  - So Double Cross keeps book_deep's line (18-14), plus the confident avoid entry 24-20 at ply 19.
- **DLL** `6bb8c6c-deep2b-egdb8` (`logs/build_deep2b.txt`): bench 7,712,020 with the book on, 8,089,571 with
  `set book off`; `dbpieces` 8, book mode 3, avoid list 6.
- **Test:** the same targeted test, alternating book_deep2b (`tests/d2bt1`–`d2bt6`) with the master book_deep
  (`tests/d1t4`–`d1t8`), 19:36–21:05.

| Book | Runs | W / D / L | Elo vs Cake on the 5 ballots [95% CI] |
|---|---|---|---|
| **book_deep2b** | 6 (600 games) | 0 / 502 / **98** | **−57.3 [−66.7, −47.8]** |
| book_deep (all 8 runs) | 8 (800 games) | 0 / 540 / 260 | −117.2 [−126.3, −108.1] |
| book_deep2 (for reference) | 3 (300 games) | 0 / 219 / 81 | −96.2 [−111.2, −81.2] |

**book_deep2b − book_deep: +59.9 Elo [+46.8, +73.0]** (LOS 100%). Against the 5 interleaved book_deep runs only, it is
+64.2 [+49.3, +79.0] (`pool_targeted_2b.txt`).

**Per ballot** (Regicide's losses / games; `per_ballot.txt`):

| Ballot | Regicide as | book_deep (8 runs) | book_deep2 (3 runs) | **book_deep2b (6 runs)** |
|---|---|---|---|---|
| ACF #7 | B | 78 / 80 | 22 / 30 | **39 / 60** |
| Wilderness II | B | 25 / 80 | 1 / 30 | **0 / 60** |
| White Doctor | B | 59 / 80 | 0 / 30 | **0 / 60** |
| Black Widow | W | 78 / 80 | 28 / 30 | 59 / 60 |
| **Double Cross** | W | 20 / 80 | 30 / 30 | **0 / 60** |

- **book_deep2b fixes three ballots completely** (Wilderness II, White Doctor, Double Cross) and halves the #7 losses.
  - **Double Cross:** with book_deep's line plus the new avoid entry (24-20 at ply 19, the move lossprobe blamed in
    that line), it went from 20/80 to 0/60.
  - **Black Widow (W)** stays lost almost every time. lossprobe found no confident blame there.
- **Caveat:** these games repeat a few lines many times, so the effective sample is small and the CI is optimistic. But
  the per-ballot changes are large and consistent across every run.
- **book_deep2b is the install candidate.** What remains for the embed condition: one full acf156 run against Cake
  (not worse than the book_deep on-runs, −4.5) and one against KingsRow, with DLL `6bb8c6c-deep2b-egdb8`. These were
  skipped for the deadline.

## Next steps (setups ready)

1. **Install candidate book_deep2b:** it passed the targeted condition (+59.9 [+46.8, +73.0]).
   - Next, run the full acf156 runs vs Cake and vs KingsRow with it: `run_ba3.sh 6bb8c6c-deep2b-egdb8 <tag>
     cake|kingsrow 0.5` in an attached `wsl.exe`.
   - Compare with the book_deep on-runs (Cake −4.5 pooled over 3 runs; KingsRow −0.0) using `pool.py`.
   - Embed if neither is worse.
2. **Black Widow (W) and #7 (B)** remain the worst cases. Ideas:
   - lossprobe at a longer movetime on the Black Widow losses (all 3 unclear at 30 s);
   - a book2-style pass only on #7 with more expansions;
   - KingsRow-database-checked lines.
3. **book_deep3** = base book + Arch's acf156 book (`t3/book-deep-acf156`): `scripts/d3_chain.sh`. Set the base to the
   chosen book.
4. **Round-7 matches** (`experiments/nnue7`).

## Files

- `book2.bin`, `book_deep2.bin`, `book2b.bin`, **`book_deep2b.bin`** (the candidate), `avoid2.bin`, `roots2.txt`, `ballots2.txt`,
  `target_openings.txt`
- `losses_bookon.pdn`, `report2.txt`
- `logs/`: `build_6bb8c6c.txt`, `lossprobe.log`, `calib.log`, `main.log`, `merge.log`, `finalize.log`,
  `build_deep2.txt`, `timeline.txt`
- `tests/`: per-run `summary.txt` and `results.csv`; `pool_targeted.txt`, `per_ballot.txt`
- `scripts/`: `d2_build.sh`, `d2_chain.sh`, `d2_extend.sh`, `d2b.sh`, `d2b_full.sh`, `merge_books.py`, `run_ba3.sh`, `pool.py`,
  `per_ballot.py`, `d3_chain.sh`

## 4. Install checks: book_deep2b, full acf156 runs (2026-11-09 21:12–21:40)

DLL `6bb8c6c-deep2b-egdb8`, settings of the deep-book tests (acf156, 312 games, 0.5 s/move, `-c 8`, 2–8 EGDB, no
background generator), run back to back (`scripts/d2b_full.sh`; `tests/deep2b-full-cake`, `tests/deep2b-full-kingsrow`).

| Opponent | book_deep2b | Reference: book_deep on-runs (`t3/book-deep`) | Difference [95% CI] |
|---|---|---|---|
| Cake | **0 / 307 / 5**, −5.6 [−10.4, −0.8] | 3 runs pooled 0 / 924 / 12, −4.5 [−6.9, −2.0] (3, 5, 4 losses per run) | −1.1 [−6.5, +4.3], LOS 34% |
| KingsRow | **0 / 312 / 0**, −0.0 | 1 / 310 / 1, −0.0 | +0.0 [−3.1, +3.1] |

- **Cake:**
  - 5 losses, inside the 3–5 range of the book_deep runs. The point estimate is 1.1 Elo below the pooled −4.5, well
    within noise.
  - The losses were #7, Gemini I, #79, #90 (Black) and Black Widow (White). None came from White Doctor, Wilderness II
    or Double Cross, the three ballots book_deep2b fixed.
- **KingsRow:** no losses (book_deep: one loss, one win).
- **Reading of the embed condition:**
  - **KingsRow passes** (0/312/0 vs 1/310/1).
  - **Cake is equal within noise** but not better than −4.5 on the point estimate (−5.6). Whether that counts as "not
    worse" is the user's call. The targeted gain (+59.9 on the 5 ballots) does not show up as a full-deck gain in one
    312-game run, because those ballots are only 10 of the 312 games.
