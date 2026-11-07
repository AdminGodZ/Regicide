# Baseline: Regicide b71dfa9 vs KingsRow, Cake, GuiNN

Regicide `t3/optimize-checkers-engine` @ `b71dfa9` (built-in gen0 eval), CheckerBoard DLL
(`regicide_cb`, llvm-mingw clang, Release) against the real Windows CheckerBoard engines, each
hosted by `cbhost.exe` (built from the same commit) and driven by `tools/match` in WSL2.

- Openings: `acf156` (156 ACF 3-move ballots, each played with both colours: 312 games).
- Time: `--movetime 0.5` (CheckerBoard engine-match rules: 0.5 s/move banked), `-c 8`.
- Regicide: hashsize 128, threads 1, book default, own 2-7 piece endgame database (copy on local disk).
- KingsRow 1.20: own opening book on, 2-8 piece WLD database (`dbmbytes 512`), hashsize 128, searchthreads 1.
- Cake 1.89f: own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- GuiNN 2.06 (avx2): own book on (default), KingsRow 2-8 piece WLD database, `dbmbytes 512`, hashsize 128.
- Adjudication: runner defaults (no legal moves, threefold repetition, 40-move no-progress rule,
  1000 plies); engine draw claims ignored.
- Machine: Intel Core Ultra 9 285K (24 cores), Windows 11 + WSL2; a 12-thread datagen job ran
  alongside (<= 22 busy threads in total).

Per opponent: `summary.txt` (runner summary, pentanomial Elo), `results.csv` (one row per game),
`losses.pdn` (every game Regicide lost, with per-move times and engine output as comments).
