# Baseline: Regicide ae210fb (NNUE nn2b, singular extensions from depth 4) vs KingsRow, Cake, GuiNN

Regicide `master` @ `ae210fb`:
- Built-in NNUE `nn2b` (FT 512, `experiments/nnue2/nn2b.nnue`), linear G2c as fallback.
- Singular extensions from depth 4.
- No opening book (`book.bin` is empty).

NNUE round 3 (`experiments/nnue3` on `t3/nnue3`) produced no net that clearly beats nn2b. nn3c (FT 1024) was
+0.9 [−4.1, +5.9] and nn3d (L1 64) −0.0 [−5.2, +5.2], so this baseline uses plain ae210fb.

The engine is the CheckerBoard DLL (`regicide_cb`, llvm-mingw clang 23.1.5, Release, the same toolchain file as the
earlier baselines), played against the real Windows CheckerBoard engines. Each engine is hosted by `cbhost.exe`, and
the games are driven by `tools/match` in WSL2. `tools/match` and `tools/cbhost` are unchanged since b71dfa9, so the
same binaries were reused (`match-747ab51`, b71dfa9 `cbhost.exe`).

The setup is the same as `results/baseline-747ab51/README.md`:
- Openings: `acf156` (156 ACF 3-move ballots, each played with both colours: 312 games).
- Time: `--movetime 0.5` (CheckerBoard engine-match rules: 0.5 s/move banked), `-c 8`.
- Regicide: hashsize 128, threads 1, own 2-7 piece endgame database (copy on local disk).
- KingsRow 1.20: own opening book on, 2-8 piece WLD database (`dbmbytes 512`), hashsize 128, searchthreads 1.
- Cake 1.89f: own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- GuiNN 2.06 (avx2): own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- Adjudication: runner defaults (no legal moves, threefold repetition, 40-move no-progress rule, 1000 plies); engine
  draw claims ignored.

Script: `run_baseline_r3.sh` (NAME=ae210fb). Build: `build_win_variant.sh`.

Machine load: Intel Core Ultra 9 285K (24 cores), Windows 11 + WSL2. A 12-thread datagen ran alongside for the whole
baseline, as in the earlier baselines:
- gen3c (825f993, nn2b) until 07:15:29, then gen3d (ae210fb, nn2b) from 07:16:17. There was a 48 s gap without
  datagen during the Cake run.
- During the KingsRow run, two extra jobs ran:
  - the FT-768 NNUE training (`nn3e`) on the GPU from 06:47 to 07:03, limited to one torch CPU thread and one loader
    thread (about 1.2 cores), as during the 747ab51 baseline;
  - before it, a 37 s `nice`d 2-job build of that FT-768 engine (06:46).

## Engine checks

- Windows build: `src/weights/net.bin` (= nn2b) and `src/weights/eval.bin` are embedded in the DLL (`BUILD.txt`).
- `regicide.exe bench` from the same Windows build gives **6,873,528 nodes**, as does the Linux build of the same tree
  (= the expected ae210fb bench with built-in nn2b). The Linux ctest passes 7/7 (`build_and_bench.txt`).

## Results

| Opponent | W / D / L | Score | Elo [95% CI] (pentanomial) | 747ab51 (nn1) | a44591e (book off) | a44591e (book on) |
|---|---|---|---|---|---|---|
| KingsRow 1.20 | 0 / 310 / 2 | 49.68% | **−2.2** [−5.3, +0.8] | −2.2 [−6.6, +2.1] | −6.7 | −43.7 |
| Cake 1.89f | 0 / 292 / 20 | 46.79% | **−22.3** [−31.5, −13.2] | −27.9 [−38.0, −17.9] | | −91.1 |
| GuiNN 2.06 | 3 / 297 / 12 | 48.56% | **−10.0** [−18.3, −1.7] | −14.5 [−23.2, −5.8] | | −66.5 |

- Pentanomial counts [LL LD DD/WL WD WW]:
  - KingsRow [0 2 154 0 0];
  - Cake [0 20 136 0 0];
  - GuiNN [0 12 141 3 0].
- Compared with 747ab51 (nn1, no singular extensions), the point estimates are better against Cake (+5.6) and GuiNN
  (+4.5), and the same against KingsRow. Each change is well inside the CIs (about ±9 for Cake and GuiNN).
- Regicide won 3 games against GuiNN, all as White.
- Most losses are as Black: Cake 17 of 20, GuiNN 11 of 12.
- Run times: KingsRow 06:43 to 07:08, Cake 07:08 to 07:31, GuiNN 07:31 to 07:55.

## KingsRow with opening book book-w03

An extra run checks whether an opening book helps now:
- **Book:** `experiments/book-w03/book_w03.bin` from `origin/t3/book-w03`, sha256 `1121af3b…`. It is a 40k-expansion
  book generated with the a44591e linear eval.
- **Build:** the CB DLL can only use an embedded book, so this is a variant DLL of the same ae210fb tree with
  that file copied to `src/weights/book.bin` (`BUILD-book-w03.txt`).
- **Settings:** the default CheckerBoard book mode (3, best moves only); otherwise exactly as the KingsRow run above.

Checks:
- **Instant book move from the start position.**
  - Through `cbhost` with the default book mode, the DLL plays 11-15 as Black and 22-17 as White.
    Both moves take 0.01 s, at depth 0 with 0 nodes. See `book_check_startpos_{1,2}.pdn`, one start-position pair vs
    the plain DLL.
  - `regicide.exe` and the Linux build of the same tree also play 11-15 at depth 0 with 0 nodes. The score is +7, the
    book's start value.
- **Bench.** The variant's bench is 6,044,379 nodes with the book on, because some bench positions are in the book.
  With `set book off` it gives 6,873,528, the same as ae210fb.

| Run | W / D / L | Elo [95% CI] (pentanomial) | pentanomial | Regicide losses (opening, colour) |
|---|---|---|---|---|
| KingsRow, no book | 0 / 310 / 2 | **−2.2** [−5.3, +0.8] | [0 2 154 0 0] | Tyne (B), Black Widow (W) |
| KingsRow, **book-w03** | 0 / 308 / 4 | **−4.5** [−8.8, −0.1] | [0 4 152 0 0] | Inferno (B), Wilderness I (B), ACF #61 (B), Black Widow (W) |

- The book was used in every game: 1,124 instant book moves in 312 games, 3.6 per game on average (at most 7), always
  at the start of Regicide's part of the game. The book-off run had 12 instant moves in 2 games; these were not book
  moves.
- **book-w03 vs no book: −2.3 Elo** (−4.5 vs −2.2), from 2 more lost games. That is within noise: the two CIs overlap
  almost entirely, and each run has only 2-4 decisive pairs.
- So book-w03 is much less harmful than book_b13, which cost about 37 Elo against KingsRow with a44591e (−43.7 vs
  −6.7). But it shows no gain, and its point estimate is slightly worse. All three extra losses (all as Black) begin
  with book moves whose book value is already negative for Regicide: Inferno −42, Wilderness I −16, ACF #61 −18. In
  each, KingsRow was still in its own book. Without the book, Regicide drew all three of those games.
- **Verdict under the "keep only if not worse" rule: not shown to be not worse, so it stays off.** A real test would
  need many more games than 312 against KingsRow (for example also vs Cake and GuiNN, or self-play with book on vs off).
- Run time: 07:55 to 08:21.

## Files

- Per opponent (`kingsrow/`, `cake/`, `guinn/`, `kingsrow-book-w03/`):
  - `summary.txt`: runner summary, pentanomial Elo;
  - `results.csv`: one row per game;
  - `losses.pdn`: every game Regicide lost, with per-move times and engine output as comments.
- `BUILD.txt`, `BUILD-book-w03.txt`: what is in each DLL.
- `build_and_bench.txt`: the build, ctest and bench log for both DLLs.
- `book_check_startpos_{1,2}.pdn`: the book check games.
- `run_baseline_r3.sh`, `build_win_variant.sh`: the run and build scripts.
