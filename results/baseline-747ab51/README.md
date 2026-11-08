# Baseline: Regicide 747ab51 (NNUE nn1) vs KingsRow, Cake, GuiNN

Regicide `master` @ `747ab51`: built-in NNUE `nn1` (`src/weights/net.bin`, `experiments/nnue1/nn1.nnue`), linear
G2c as fallback, no opening book (`book.bin` is empty). CheckerBoard DLL (`regicide_cb`, llvm-mingw clang 23.1.5,
Release, same toolchain file as the earlier baselines) against the real Windows CheckerBoard engines. Each engine is
hosted by `cbhost.exe` and the games are driven by `tools/match` in WSL2. `tools/match` and `tools/cbhost` are
unchanged since b71dfa9, and the b71dfa9 `cbhost.exe` was reused.

The setup is the same as `results/baseline-a44591e/README.md`:
- Openings: `acf156` (156 ACF 3-move ballots, each played with both colours: 312 games).
- Time: `--movetime 0.5` (CheckerBoard engine-match rules: 0.5 s/move banked), `-c 8`.
- Regicide: hashsize 128, threads 1, own 2-7 piece endgame database (copy on local disk). There is no book, so the
  book setting has no effect.
- KingsRow 1.20: own opening book on, 2-8 piece WLD database (`dbmbytes 512`), hashsize 128, searchthreads 1.
- Cake 1.89f: own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- GuiNN 2.06 (avx2): own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- Adjudication: runner defaults (no legal moves, threefold repetition, 40-move no-progress rule, 1000 plies); engine
  draw claims ignored.

Machine load: Intel Core Ultra 9 285K (24 cores), Windows 11 + WSL2. A 12-thread datagen ran alongside for the whole
baseline, as in the earlier baselines:
- gen3a (bd2ef1d, linear G2c) until 02:49:30, then gen3b (747ab51, NNUE) from 02:49:38.
- The NNUE round-2 training on the GPU also ran throughout, limited to one torch CPU thread and one loader thread
  (about 1.2 cores), so the total stayed at about 21-22 busy threads.
- Two short exceptions during the KingsRow run, about games 20-40 (01:48 to 01:51):
  1. Two 1-minute trainer CPU probes used about 10 cores each.
  2. A `nice`d 2-job build of the FT-512 scratch engine ran.

DLL checks before the runs: `src/weights/net.bin` (= nn1) and `src/weights/eval.bin` are embedded in the DLL.
`regicide.exe bench` from the same Windows build gives **4,082,499 nodes**, the same as the Linux build and as nn1's
bench in `experiments/nnue1` (`bench_windows.txt`, `bench_linux.txt`). Through `cbhost`, the DLL reports 7 database
pieces and searches the start position (no book).

## Results

| Opponent | W / D / L | Score | Elo [95% CI] (pentanomial) | a44591e (book on) | a44591e (book off) | b71dfa9 |
|---|---|---|---|---|---|---|
| KingsRow 1.20 | 1 / 308 / 3 | 49.68% | **−2.2** [−6.6, +2.1] | −43.7 | −6.7 | −155.5 |
| Cake 1.89f | 0 / 287 / 25 | 45.99% | **−27.9** [−38.0, −17.9] | −91.1 | | −269.8 |
| GuiNN 2.06 | 2 / 295 / 15 | 47.92% | **−14.5** [−23.2, −5.8] | −66.5 | | −254.7 |

- Pentanomial counts [LL LD DD/WL WD WW]: KingsRow [0 3 152 1 0], Cake [0 25 131 0 0], GuiNN [0 15 139 2 0].
- Against KingsRow the result is not significantly different from equal (LOS 15.8%). Regicide won one game against
  KingsRow and two against GuiNN.
- Run times: KingsRow 01:46 to 02:10, Cake 02:10 to 02:32, GuiNN 02:32 to 02:57.

Per opponent (`kingsrow/`, `cake/`, `guinn/`): `summary.txt` (runner summary, pentanomial Elo), `results.csv` (one row
per game), `losses.pdn` (every game Regicide lost, with per-move times and engine output as comments).
