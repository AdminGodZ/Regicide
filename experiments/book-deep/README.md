# book-deep: deep opening book targeted at the ballots Regicide loses (master ff0a3d2, built-in n6c)

**Result: the deep book halves Regicide's losses to Cake. Over three interleaved on/off pairs (936 games each), book on
− off = +4.5 Elo [+0.2, +8.7]** (95% CI, pentanomial; LOS 97.9%).
- **Cake:** book on 0 / 924 / 12 (−4.5 [−6.9, −2.0]), book off 0 / 912 / 24 (−8.9 [−12.4, −5.4]). Every pair favours the
  book: 3 vs 6, 5 vs 12, 4 vs 6 losses.
- **Not worse elsewhere** (same-engine references with the book off):
  - KingsRow on −0.0 (1/310/1) vs off −3.3 (0/309/3), +3.3 [−1.5, +8.2];
  - GuiNN on −1.1 (1/309/2) vs off −3.3 (2/305/5), +2.2 [−4.7, +9.1].
- **All five on/off pairs combined:** +3.8 [+0.7, +6.9], LOS 99.2%.
- **So both conditions for embedding are met:** the pooled Cake CI lower bound is above 0, and KingsRow and GuiNN are
  not worse.
- **The losses are where the book points:** every Cake loss, with or without the book, came from one of the 46 root
  ballots.

## The book

- **Engine:** `bookgen` of master **ff0a3d2** (built-in NNUE n6c; bench 8,089,571, ctest 13/13; `logs/build_ff0a3d2.txt`),
  so the book matches the engine it ships with.
- **Roots** (`roots.txt`, one FEN per line; built by `scripts/deep_roots.py`): the position after the 3-move ballot,
  White to move, of every ballot Regicide lost.
  - **Runs considered:** 27 CheckerBoard runs of the n4c-family engine on 2026-11-08/09, 160 lost games:
    - `baseline-eb1037c-n4c` (7-piece, incl. 2 s) and `baseline-30f09c3-egdb8`;
    - the 7-piece control;
    - all `book-avoid` runs (65f7143, 769ef0b);
    - the Cake 2 s run and repeat;
    - the TM runs.
  - **46 ballots, 46 distinct roots:** 42 lost only as Black, 4 only as White (Double Cross, Black Widow, ACF #112,
    ACF #129).
  - The full list with loss counts per ballot and opponent is in `roots_ballots.txt`. The most-lost ballots:

| Ballot | Moves | Losses (colour) |
|---|---|---|
| ACF #6 Inferno | 9-13 22-18 10-14 | 13 (B) |
| ACF #62 Gemini I | 10-14 22-17 11-16 | 11 (B) |
| ACF #46 Double Cross | 9-14 23-18 14x23 | 10 (W) |
| ACF #153 Black Widow | 11-16 23-19 16x23 | 9 (W) |
| ACF #16 Wilderness II | 9-13 23-18 11-16 | 9 (B) |
| ACF #68 White Doctor | 10-14 22-18 12-16 | 8 (B) |
| ACF #63 | 10-14 22-17 14-18 | 7 (B) |
| ACF #9 Wilderness I | 9-13 22-18 11-16 | 6 (B) |
| ACF #14, ACF #141, ACF #7 | | 5 each (B) |

  The other 35 ballots have 1–4 losses each.

- **Command:** `bookgen --roots roots.txt --nodes 30000000 --threads 20 --out book_deep.bin`.
  - **30M nodes per analysed move**, against 1M for `book_n4c`, the same as about one 0.5 s game move.
  - Default drop and max-ply.
- **Running time:**
  - **Calibration** (`logs/calib.log`), 03:11–03:19: the 46 roots took 178 s, then 80 expansions took 322 s, so
    **0.248 expansions/s** on 20 threads.
  - The first main run was started with N computed from a wrong rate (the roots analysis was counted as expansion
    time) and stopped after a minute, before its first save (`logs/main.aborted-0319.log`). Nothing was lost.
  - **Main run** (`logs/main.log`), 03:20–09:30: `--resume` from the calibration book, `--expansions 5500`, steady at
    about 0.245/s. In total **5,520 expansions, 6.3 h**.
  - The 50k-node datagen (gen5c) yielded to bookgen for the whole run, and nothing else ran.
- **Result:** `book_deep.bin` holds **5,646 positions** (the first root's value ended at +39; `logs/main.log`).

## Engine for the tests

- **DLL `ff0a3d2-deep-egdb8`:** master ff0a3d2 with `book_deep.bin` embedded as `src/weights/book.bin`, full 2–8
  piece EGDB (`logs/build_deep.txt`, `logs/BUILD-ff0a3d2-deep-egdb8.txt`).
- **Checks:**
  - bench 7,712,020 with the book on (Windows and Linux), and **8,089,571 with `set book off`** (= ff0a3d2);
  - through `cbhost`: `dbpieces` 8, book mode 3 (default: best moves only).
- **Book off** is the same DLL with `--option-a "set book 0"`.

## Tests

Settings follow the baselines:
- `acf156`, 312 games, 0.5 s/move (CheckerBoard banked), `-c 8`;
- Regicide hash 128, threads 1, no avoid list;
- opponents with their books and the KingsRow 2–8 WLD database;
- `match-747ab51` with b71dfa9 `cbhost.exe`;
- gen5c datagen (50k nodes, 12 threads) alongside, as in the earlier baselines;
- run times: 09:36–10:29.

| Run | W / D / L | Pentanomial | Elo [95% CI] | Games with instant moves (book or EGDB) | Losses (ballot, Regicide's colour) |
|---|---|---|---|---|---|
| Cake, book off | 0 / 306 / 6 | [0 6 150 0 0] | −6.7 [−11.9, −1.4] | 6 | #7 (B), Double Cross (W), Gemini I (B), White Doctor (B), Black Widow (W), Skunk (B) |
| **Cake, book on** | 0 / 309 / **3** | [0 3 153 0 0] | **−3.3 [−7.1, +0.4]** | 165 | #7 (B), Wilderness II (B), Black Widow (W) |
| KingsRow, book on | 1 / 310 / 1 | [0 1 154 1 0] | −0.0 [−3.1, +3.1] | 161 | Black Widow (W) |
| GuiNN, book on | 1 / 309 / 2 | [0 2 153 1 0] | −1.1 [−4.9, +2.7] | 161 | Double Cross (W), #63 (B) |

- **Book on vs off against Cake, first pair** (`pool.py` on the two runs): +3.3 Elo [−3.1, +9.8], LOS 84%. Pairs 2
  and 3 and the pooled result are below.
- **The book was used in about 160 games per run.** That is more than the 92 games played on the 46 root ballots,
  because the book also covers positions reached from other ballots by transposition. Each run had about 470–520
  instant moves, a few of them EGDB root moves.
- **All 6 + 3 Cake losses are on root ballots.** The book removed the Gemini I, White Doctor, Skunk and Double Cross
  losses. #7 and Black Widow were lost both with and without the book, and Wilderness II was lost only with it.
- **KingsRow and GuiNN are not worse with the book.**
  - KingsRow −0.0: the off runs of the n4c engine gave −0.0 and −3.3; n6c had no off run against KingsRow.
  - GuiNN −1.1: n4c off gave −3.3.

## More pairs against Cake, and same-engine references (10:42–12:00)

The same DLL and settings were used, in the order off, on, off, on (Cake pairs 2 and 3), then KingsRow off and GuiNN
off.

| Run | W / D / L | Pentanomial | Elo [95% CI] | Losses (ballot, Regicide's colour) |
|---|---|---|---|---|
| Cake off, pair 2 | 0 / 300 / 12 | [0 12 144 0 0] | −13.4 [−20.7, −6.1] | Dreaded Edinburgh, #7, Wilderness I, Twilight Zone, Gemini I, White Doctor, #141, Skunk (B); Double Cross, #112, #129, Black Widow (W) |
| Cake on, pair 2 | 0 / 307 / 5 | [0 5 151 0 0] | −5.6 [−10.4, −0.8] | #7, Wilderness II, White Doctor (B); Double Cross, Black Widow (W) |
| Cake off, pair 3 | 0 / 306 / 6 | [0 6 150 0 0] | −6.7 [−11.9, −1.4] | #7, Twilight Zone, Gemini I, Skunk (B); Double Cross, Black Widow (W) |
| Cake on, pair 3 | 0 / 308 / 4 | [0 4 152 0 0] | −4.5 [−8.8, −0.1] | #7, White Doctor (B); Double Cross, Black Widow (W) |
| KingsRow off | 0 / 309 / 3 | [0 3 153 0 0] | −3.3 [−7.1, +0.4] | Inferno, #25, Black Hole (B) |
| GuiNN off | 2 / 305 / 5 | [0 5 149 2 0] | −3.3 [−9.1, +2.4] | White Doctor, Skull-Cracker, #152, #157, Skunk (B) |

**Pooled** (`pool.py`, `pool_cake.txt`): Elo from the pentanomial pair scores, normal approximation. The difference
treats the two pools as independent samples.

| Pool | Games | W / D / L | Pentanomial | Elo vs opponent [95% CI] |
|---|---|---|---|---|
| Cake, book on (3 runs) | 936 | 0 / 924 / 12 | [0 12 456 0 0] | −4.5 [−6.9, −2.0] |
| Cake, book off (3 runs) | 936 | 0 / 912 / 24 | [0 24 444 0 0] | −8.9 [−12.4, −5.4] |
| **Cake on − off** | | | | **+4.5 [+0.2, +8.7]**, LOS 97.9% |
| KingsRow on − off | 312 + 312 | 1/310/1 vs 0/309/3 | | +3.3 [−1.5, +8.2], LOS 91.1% |
| GuiNN on − off | 312 + 312 | 1/309/2 vs 2/305/5 | | +2.2 [−4.7, +9.1], LOS 73.6% |
| all five pairs, on − off | 1,560 + 1,560 | | | +3.8 [+0.7, +6.9], LOS 99.2% |

- **Book-on Cake losses** over the three runs: ACF #7 (B) and Black Widow (W) 3 times each; Double Cross (W) and White
  Doctor (B) twice each; Wilderness II (B) twice.
- **Never lost with the book**, but lost without it: Gemini I, Skunk, Twilight Zone, Dreaded Edinburgh,
  Wilderness I, #141 (all B), and #112, #129 (W).
- **Remaining weak spots:** #7 and Black Widow still lose in every run, with or without the book. They are the
  candidates for a deeper second pass: more expansions or more nodes on those roots.

## The net (n6c) against Cake

The book-off run is also the first Cake run of the new built-in net n6c (`origin/t3/nnue6`):
- **n6c, no book: −6.7 [−11.9, −1.4]** (0/306/6);
- n4c, no book, pooled over three runs (`results/book-avoid-769ef0b` off, `results/cake-analysis-769ef0b` repeat,
  `experiments/tm-tune` tm-off): −11.9 [−15.9, −7.9] (0/904/32).
- **n6c − n4c: +5.2 Elo [−1.4, +11.8]**, LOS 94%. These are different builds (ff0a3d2 vs 769ef0b/b88c02e), and the
  only engine difference is the net.

## Next

- **Pool more games against Cake,** for example 2 × 312 more each with the book on and off, to settle the +3.3.
- **Extend the book** to the ballots that still lose (#7, Black Widow, Wilderness II, Double Cross) with more
  expansions per root, or deeper (`--nodes 100M`) for the top 10 roots.

## Files

- `book_deep.bin`, `roots.txt`, `roots_ballots.txt`, `roots.out`
- `logs/`:
  - `calib.log`, `main.log`, `main.aborted-0319.log`: bookgen output with Unix timestamps;
  - `build_ff0a3d2.txt`, `build_deep.txt`, `BUILD-ff0a3d2-deep-egdb8.txt`;
  - `timeline.txt`.
- `tests/deep-{off,on}{,2,3}-cake`, `tests/deep-{off,on}-kingsrow`, `tests/deep-{off,on}-guinn`: `summary.txt`,
  `results.csv`, `losses.pdn`, `runner.log`.
- `scripts/`: `deep_chain2.sh` (the book and the first tests), `deep_more.sh` (pairs 2-3, references), `pool.py`,
  `pool_cake.txt`, `deep_roots.py`, `run_ba2.sh`, `build_win_variant.sh`.
