# Baseline: Regicide a44591e vs KingsRow, Cake, GuiNN

Regicide `master` @ `a44591e`: built-in eval `w_lr0.03_wd0`, embedded 40k-expansion opening book (`book_b13`).
CheckerBoard DLL (`regicide_cb`, llvm-mingw clang 23.1.5, Release, same toolchain file as the b71dfa9 build) against the
real Windows CheckerBoard engines. Each engine is hosted by `cbhost.exe` and the games are driven by `tools/match` in
WSL2. The `tools/match` and `tools/cbhost` sources are unchanged between b71dfa9 and a44591e; the b71dfa9 `cbhost.exe`
was reused.

The setup is the same as `results/baseline-b71dfa9/README.md`:
- Openings: `acf156` (156 ACF 3-move ballots, each played with both colours: 312 games).
- Time: `--movetime 0.5` (CheckerBoard engine-match rules: 0.5 s/move banked), `-c 8`.
- Regicide: hashsize 128, threads 1, book default (mode 3, best moves only), own 2-7 piece endgame database (copy on
  local disk). For `kingsrow-bookoff`, Regicide also gets `set book 0`; nothing else changes.
- KingsRow 1.20: own opening book on, 2-8 piece WLD database (`dbmbytes 512`), hashsize 128, searchthreads 1.
- Cake 1.89f: own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- GuiNN 2.06 (avx2): own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- Adjudication: runner defaults (no legal moves, threefold repetition, 40-move no-progress rule, 1000 plies); engine
  draw claims ignored.
- Machine: Intel Core Ultra 9 285K (24 cores), Windows 11 + WSL2. A 12-thread datagen job (gen2b) ran alongside the
  first three runs, as in the b71dfa9 baseline. It finished at 23:15, during the GuiNN run (started 22:53), so the second
  part of GuiNN and all of `kingsrow-bookoff` (23:17-23:40) ran on an otherwise idle machine.

DLL checks before the runs: the bytes of `src/weights/eval.bin` and `src/weights/book.bin` are both embedded in the DLL.
`regicide.exe bench` from the same Windows build gives the same node counts as a Linux build of a44591e
(3,278,128 nodes; `bench_linux.txt`, `bench_windows.txt`). Through `cbhost`, the DLL plays the start position at once
from the book (`9-13`, 0 nodes, 9 ms) and reports 7 database pieces. With `set book 0` it searches instead.

## Results

| Opponent | W / D / L | Score | Elo [95% CI] (pentanomial) | b71dfa9 baseline Elo | Change |
|---|---|---|---|---|---|
| KingsRow 1.20 | 0 / 273 / 39 | 43.75% | **−43.7** [−56.9, −30.5] | −155.5 | +111.8 |
| Cake 1.89f | 0 / 232 / 80 | 37.18% | **−91.1** [−107.3, −75.3] | −269.8 | +178.7 |
| GuiNN 2.06 | 1 / 251 / 60 | 40.54% | **−66.5** [−82.1, −51.2] | −254.7 | +188.2 |
| KingsRow 1.20, Regicide book off | 0 / 306 / 6 | 49.04% | **−6.7** [−11.9, −1.4] | | |

Book effect vs KingsRow: book off −6.7, book on −43.7. Regicide scored **+37 Elo better without its book**: 6 losses
instead of 39, and pentanomial [0 6 150 0 0] vs [3 33 120 0 0]. The book was generated with the old B13 eval
(`experiments/gen1full`), and from the start of the game it plays B13's choices instead of searching with the current
eval. Each of the 4 runs is a single 312-game sample, and the two KingsRow runs use the same openings. The intervals
do not overlap.

Per run (`kingsrow/`, `cake/`, `guinn/`, `kingsrow-bookoff/`): `summary.txt` (runner summary, pentanomial Elo),
`results.csv` (one row per game), `losses.pdn` (every game Regicide lost, with per-move times and engine output as
comments).

Note: the PC lost power at about 23:45, after all 4 runs had finished (the last ended 23:39:51). All 4 result sets were
checked afterwards: 312 of 312 rows, games 1..312 present, summaries intact.
