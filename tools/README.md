# Benchmarking tools

Native opponents and a match runner, so Regicide can be measured against strong engines on
macOS. Cake and KingsRow are Windows-only CheckerBoard DLLs; here we build the strongest
open engines natively and play them through the CheckerBoard engine API. Regicide exports the
same API: the top-level CMake target `regicide_cb` builds it as a shared library
(`build/libregicide_cb.dylib` on macOS), which `match` loads through `cbhost` like any other engine.

| Path | What |
|---|---|
| `common/` | Self-contained English checkers referee (movegen, FEN/PDN, CheckerBoard board and time packing). Independent of `src/`. |
| `cbhost/` | `cbhost`: loads one CheckerBoard-API engine (dylib/so, or a 64-bit DLL on Windows) per process and speaks a line protocol ([PROTOCOL.md](cbhost/PROTOCOL.md)). |
| `match/` | `match`: the match runner. |
| `opponents/` | Scripts that fetch, patch and build the opponent engines into `build/opponents/`. |

## Build

```sh
cmake -S . -B build -G Ninja && cmake --build build   # builds cbhost, match, referee_test
ctest --test-dir build                                # includes the referee perft test
tools/opponents/build_all.sh                          # simplech, Cake 1.22, GuiNN 2.06
```

The tools also build on Linux, including WSL on Windows: engines are then `.so` files instead
of `.dylib`, and x86-64 builds use GuiNN's original AVX2 code. Each opponent script clones the upstream repository at a pinned commit into
`build/opponents/src/`, applies our patch, and writes the dylib and its data files to
`build/opponents/<engine>/`. No upstream source is committed here (Cake is GPL, GuiNN is
CC BY-NC-SA). `build_guinn.sh` also runs the NN equivalence check (below).

| Engine | Script | Source | Notes |
|---|---|---|---|
| GuiNN Checkers 2.06 | `build_guinn.sh` | [eygilbert/GuiNN_Checkers](https://github.com/eygilbert/GuiNN_Checkers) `615336a` | Neural-net eval, the strongest opponent. Nets `Nets206.gnn`, book `opening.gbk`, 4-piece db `database.jef`. |
| Cake 1.22 | `build_cake.sh` | [hungerburg/capers](https://github.com/hungerburg/capers) `90cf176`, `cake/` | GPL fork of Cake 1.4. Book `xbook.bin`, Chinook 4-piece db. |
| Simple Checkers 1.15 | `build_simplech.sh` | [eygilbert/CheckerBoard](https://github.com/eygilbert/CheckerBoard) `32f194f`, `source/simplech.c` | Weak sanity opponent. |

## Running a match

```sh
build/tools/match -a build/opponents/cake/cake.dylib -b build/opponents/guinn/guinn.dylib \
    --movetime 0.2 -c 6 -o build/matches/cake-vs-guinn
```

`-a` is the engine under test; everything is reported from its point of view. An engine is a
CheckerBoard `.dylib` (run through `cbhost`) or `exec:COMMAND` for a program that speaks the
cbhost protocol itself (the `regicide` CLI does not). `match --help` lists all options;
the important ones:

- `--openings acf156|acf174|acf144|FILE`. Every opening is played twice with colours reversed.
  `acf156` (default) is the ACF 3-move deck without the 18 barred ballots (144 cross-board +
  12 mail-play), the usual tournament set; `acf174` adds the barred ones; `acf144` is
  CheckerBoard's default (cross-board only). The deck is CheckerBoard's `three_move_table`
  (`utility.c`), in ACF order. A file can hold FENs, move sequences from the start position,
  `FEN | moves`, or PDN games (FEN tag + movetext).
- Time control, with CheckerBoard semantics so engines see exactly what CheckerBoard sends:
  - `--movetime S` (default 0.1): fixed time per move as in a CheckerBoard engine match: S is
    added to a per-side bank each move, and `maxtime = S + (bank - S) / 2.5`. `--movetime-mode
    plain` sends `maxtime = S`; `exact` also sets `CB_EXACT_TIME`.
  - `--tc 60+0.5`: initial time + increment, packed into `info`/`moreinfo` like CheckerBoard's
    `format_time_args()`, with `maxtime` from `maxtime_for_incremental_tc()` for engines that
    ignore the packed times. Clocks start at initial + increment. Games are forfeited on time
    (`--no-time-forfeit` to disable).
  - `CB_RESET_MOVES` is set on each side's first search of a game, and the game history goes
    to engines via `enginecommand("set gamehist ...")` before every search, as in CheckerBoard.
- Adjudication: no legal moves loses; threefold repetition draws; `--no-progress 40` draws
  after 40 moves by one side and 39 by the other with no capture or man move (CheckerBoard's
  rule); `--max-plies 1000`. Engine result claims are ignored unless `--accept-draw-claims`
  (CheckerBoard ends engine-match games on a draw claim) or `--cb-claims` (CheckerBoard with
  its default early adjudication: a draw, win or loss claim ends the game after its move);
  they are always recorded (PDN tags `FirstClaim`, `ClaimsA`, `ClaimsB`, results.csv
  `first_claim`). `--cb-move-limit N` ends a game still running after N engine moves with
  result `*`, not scored (CheckerBoard: 200 with early adjudication). `Adjudicator` in
  `match/main.cpp` is the hook for endgame-database adjudication.
- Robustness: every move is validated by the referee (identified by from/to/captured set,
  so multi-jump paths are never ambiguous). Illegal moves lose. An engine that does not answer
  gets `stop` after `--stop-factor` x maxtime + `--grace` (fixed time) or its remaining clock
  (increment), and is killed `--kill-after` seconds later: loss, process restarted. A crashed
  engine loses the game and is restarted.
- `--option-a`/`--option-b TEXT`: engine commands sent after startup (`set hashsize 64`). If an
  engine rejects one (its reply starts with `reply 0`), the match does not start, so an engine
  never plays with settings other than the ones asked for. `--allow-rejected-options` prints a
  warning and plays on.
- `--sprt ELO0,ELO1[,ALPHA,BETA]`: stop early when the GSPRT decides (never before
  `--sprt-min-pairs`, default 20).
- `-c N` games in parallel (default 6; two processes each). The CheckerBoard engines here are
  single-threaded. Use fewer games than performance cores for time-based matches.

Output directory: `games/NNNNN.pdn` (one file per game), `games.pdn` (all games),
`results.csv`, `summary.txt`, and `logs/` (each engine process's stderr). The runner prints
one line per finished game with the running score.

### Statistics

The unit is the game pair (same opening, colours reversed), whose score for A is 0, 1/2, 1,
3/2 or 2: the pentanomial model. Pairing cancels most of the opening bias, which matters in
checkers, where many ballots are lopsided and most games between strong engines are draws.

- Score, W/D/L, draw rate, results by colour.
- Elo = -400 log10(1/s - 1) of the mean score s; the 95% interval maps s +- 1.96 SE through
  the same formula, with SE from the variance of the pair scores. The trinomial (per-game)
  estimate is printed for comparison; it overstates confidence when pairs are correlated.
- LOS = Phi((s - 0.5) / SE).
- GSPRT: LLR = N (s1 - s0)(2s - s0 - s1) / (2 var), with N pairs, s the mean pair score, and
  s0, s1 the scores of elo0, elo1 (normal approximation). var is the per-pair variance with two
  pseudo-pairs added, a quarter point above and below the mean: var = (N v + 2 * 0.25^2) / (N + 2),
  where v is the variance of the N pair scores. Without them a run of identical pairs would have
  variance 0. Bounds log(beta/(1-alpha)) and log((1-beta)/alpha).
- As `--help` says: with very few decisive pairs the GSPRT's error rates are not guaranteed, and a
  long run of drawn pairs can accept elo0 too easily. Use a fixed number of pairs for decisions.

## Engine port notes

- **clock()**: CheckerBoard engines time their searches with `clock()`, which is wall time on
  Windows but CPU time on POSIX. `opponents/compat/cbport.h` maps it to a monotonic wall clock.
- **Cake 1.22**: upstream disabled the endgame database on 64-bit targets because the Chinook
  db code assumes 32-bit `long` (it also passed a struct with 64-bit fields to the db). The
  patch makes `db.c` LP64-clean and re-enables it; Cake then reports DB wins in 4-piece
  endings. Its `getmove` did not write the new position back into the board (CheckerBoard
  identifies the move from the board); the patch adds that, and passes `CB_RESET_MOVES`
  through (upstream never reset its repetition history).
- **GuiNN 2.06**: the NN inference SIMD (`NeuralNet/mathSimd.h`) gets NEON versions that keep
  the AVX2 integer semantics (wrapping int16 accumulators, saturating 32-to-16 pack, ReLU);
  the x86 paths are unchanged. `build_guinn.sh` builds `nn_check`, which evaluates 4000
  positions from random games (all four game-phase nets) three ways: full evaluation,
  incremental first-layer updates as in search, and an independent scalar reference. All must
  agree exactly. On Apple silicon it also builds the original AVX2 code for x86_64 and runs it
  under Rosetta: the two builds' outputs are byte-identical. The KingsRow egdb driver is not
  available, so the egdb hook loads GuiNN's own 4-piece database (`database.jef`), as the
  standalone program does; settings are not persisted (no registry), defaults are 128 MB hash
  and book on.
- **simplech**: compiled as C++ with C linkage for the exports; its My Documents log is off.
