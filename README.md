# Regicide

An 8x8 English checkers (draughts) engine in C++26, built to eventually compete with Cake and KingsRow.

## Build

```sh
cmake -S . -B build -G Ninja      # Release, -march=native and LTO by default
cmake --build build
ctest --test-dir build            # unit and regression tests
./build/regicide
```

## Commands

```
position startpos | position fen <FEN>   e.g. position fen W:WK14:B10,11,18,19
d                                         print board + FEN
moves / play <index>                      list / play legal moves
perft <n> / divide <n>                    move generator check
go depth <n> | movetime <ms> | nodes <n>  search; prints bestmove
help                                      all commands and options
```

Moves are written with every landing square of a capture (`move_text` in `src/movegen.hpp`), so `bestmove` and
`moves` output reads back as the same move with `play`, also where two captures share their end squares.

## Benchmarking

`tools/` has a CheckerBoard engine host (`cbhost`), a match runner (`match`) and scripts that
build native opponents (GuiNN 2.06, Cake 1.22, simplech). See [tools/README.md](tools/README.md).

```sh
tools/opponents/build_all.sh
build/tools/match -a build/opponents/cake/cake.dylib -b build/opponents/guinn/guinn.dylib --movetime 0.2
```

## Opening book and avoid list

The book file `src/weights/book.bin` (RGBK) is embedded at build time. The checked-in file is the current opening
book, 34,296 entries and 6 avoid entries (548,848 bytes); an empty file builds an engine without a book. At run
time the book is switched off with `set book off` in the CLI or the engine command `set book 0` in CheckerBoard and
the match runner. The book file can also carry an
**avoid list**: (position, move) pairs the engine never plays at the root, unless every legal move is on it. Book
probes never return an avoided move either; positions decided by the endgame database ignore the list. Books without
avoid entries are still written as version 1; with them, version 2 (see `src/book.hpp`).

- CLI: `set avoidfile <path|builtin>` (the avoid list of any book file), `set avoid 0|1`.
- CheckerBoard DLL / match runner: the same as engine commands, e.g.
  `--option-a "set avoidfile C:\regicide-match\avoid.bin"`, `--option-a "set avoid 0"`. On by default (built-in list).
- `selfplay --avoida FILE --avoidb FILE` (off by default).

**1. Find the losing moves** (`build/lossprobe`). Each lost game is walked from the end of the ballot; every Regicide
position (forced moves skipped) is searched twice: over all moves, then the played move alone (or, if it was the best,
the other moves). The first move whose score drops at least `--drop` (30) below the previous Regicide position's
score, with an alternative at least `--margin` (20) better, while the position was not yet lost (`--lost`, 300, or a
proven loss) is blamed; otherwise the game is reported as unclear. The run below (103 lost games from the three newest
baselines, ~1,700 analysed positions, two searches each) takes roughly 9 h with `--jobs 3` on a 24-core machine; halve
`--movetime` to halve it.

```sh
cmake --build build --target lossprobe bookgen
R=results
build/lossprobe \
  --pdn $R/baseline-eb1037c-n4c/{kingsrow,kingsrow-2s,cake,guinn}/losses.pdn \
  --pdn $R/baseline-ae210fb/{kingsrow,cake,guinn}/losses.pdn \
  --pdn $R/baseline-747ab51/{kingsrow,cake,guinn}/losses.pdn \
  --movetime 30000 --threads 8 --jobs 3 --hash 1024 --egdb ~/regicide-data/egdb \
  --out avoid.bin --report report.txt
```

Regicide's side comes from the Black/White headers (`--name Regicide`, or `--side black|white`), the ballot length
from the Opening header (else `--ballot 3`). `--nodes N` / `--depth D` replace `--movetime`; `--merge old.bin` adds
the entries of an earlier avoid list. Check `report.txt` (ply, FEN, played move and score, best alternative and score,
PVs) before using the list.

**2. Build the book with it.** `bookgen --avoid` never analyses, adds or expands avoided moves and writes them into
the book; `--resume` keeps a book's avoid entries.

```sh
build/bookgen --nodes 3000000 --expansions 20000 --threads 22 --avoid avoid.bin --out book.bin
```

**3. Embed** the book (or `avoid.bin` alone, for an avoid list without book moves) and rebuild:

```sh
cp book.bin src/weights/book.bin      # or: cp avoid.bin src/weights/book.bin
cmake --build build && ctest --test-dir build
build/avoid_test | head -1            # "built-in book: N entries, M avoid entries"
```

The bench signature (`regicide bench`) changes when book moves cover bench positions; an avoid-only file leaves it
unchanged unless a bench position is on the list.

## Layout

| File | Purpose |
|---|---|
| `src/types.hpp` | 32-bit bitboard, diagonal shifts, PDN square mapping |
| `src/position.*` | Board state, PDN FEN, make move |
| `src/movegen.*` | Legal moves: compulsory captures, multi-jumps, crowning ends the move |
| `src/perft.*` | Node counting for move generation tests |
| `src/eval.*` | Learned linear pattern eval (RGEV weights), hand-crafted fallback, evaluator dispatch |
| `src/nnue.*` | NNUE inference (RGNN nets): quantised forward pass, incremental accumulators |
| `tools/nnue/` | PyTorch NNUE trainer and export check, see [tools/nnue/README.md](tools/nnue/README.md) |
| `src/search.*` | Iterative-deepening alpha-beta search, time management, database play and the conversion search |
| `src/egdb/` | Endgame database: generator, compressed storage and probes, see [src/egdb/README.md](src/egdb/README.md) |
| `src/book.*` | Opening book and avoid list (RGBK), embedded from `src/weights/book.bin` |
| `tools/bookgen.cpp` | Opening book generator (best-first drop-out expansion) |
| `tools/lossprobe.cpp` | Lost-game analysis: blames the losing move, writes an avoid list |

Square indices are 0..31 with 0 = a1; PDN numbering (1 = b8, 32 = g1) is used for all I/O.

## Endgame conversion

With the endgame database loaded (`set egdb <dir>`), a decided root within the database's moves-to-conversion
(MTC) tables (up to 6 pieces) is played from those tables without a search. The database has win/draw/loss values
up to 8 pieces but MTC only up to 6, so a won 7- or 8-piece root is played by a conversion search: only moves that
keep the database win are searched, and won and lost positions are scored by a progress eval (material,
advancement, king distance to the loser's pieces, the no-progress count) instead of being cut off as equal wins.
Before, the engine played the first winning move that did not repeat a position, and drew many such games by
repetition or the 40-move rule. In the conversion tests the engine now wins 400 of 400 sampled 3-piece wins without
MTC (155 before) and 199 of 200 real 7-piece wins at 100k nodes per move. See `Engine::go` and `Worker::search` in
`src/search.cpp`, and [experiments/wld-conversion/RESULTS.md](experiments/wld-conversion/RESULTS.md) for the design
and measurements.

## Implemented

- Search: transposition table with Zobrist keys, iterative deepening with aspiration windows, principal variation
  search, killers, history and continuation history, LMR (per engine), late-move and futility pruning, enhanced
  transposition cutoffs, singular extensions, Lazy SMP, time management for fixed and CheckerBoard time controls.
- The 79-ply no-progress draw (`NO_PROGRESS_PLIES`) and repetitions are scored as draws inside the search, so the
  engine sees CheckerBoard's draw rules coming.
- `SearchResult::sound_score()`: the score as a statement about the position; a losing score from a cut-short
  iteration does not count as a proven loss. bookgen, datagen, selfplay and lossprobe use it.
- Evaluation: a learned linear pattern eval (RGEV) and NNUE (RGNN) with incremental accumulators and SIMD (AVX2,
  NEON); trainer in `tools/nnue/`, self-play data from `tools/datagen.cpp`.
- Endgame database: WLD for 2–8 pieces and MTC for 2–6, generated by `egdb_gen`; database play and the conversion
  search above.
- Opening book with avoid list, `bookgen` and `lossprobe`.
- CheckerBoard engine library (`regicide_cb`) and a match runner that plays it against other CheckerBoard engines;
  the runner refuses to start when an engine rejects one of its options (`--allow-rejected-options` overrides).

## Not implemented

- Probing KingsRow or Cake format endgame databases (8–10 pieces); Regicide uses its own database only.
