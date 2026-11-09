# Regicide

An 8x8 English checkers (draughts) engine in C++26, built to eventually compete with Cake and KingsRow.

## Build

```sh
cmake -S . -B build -G Ninja      # Release, -march=native and LTO by default
cmake --build build
ctest --test-dir build            # perft(0..10) against known counts
./build/regicide
```

## Commands

```
position startpos | position fen <FEN>   e.g. position fen W:WK14:B10,11,18,19
d                                         print board + FEN
moves / play <index>                      list / play legal moves
perft <n> / divide <n>                    move generator check
go depth <n>                              iterative-deepening alpha-beta
```

## Benchmarking

`tools/` has a CheckerBoard engine host (`cbhost`), a match runner (`match`) and scripts that
build native opponents (GuiNN 2.06, Cake 1.22, simplech). See [tools/README.md](tools/README.md).

```sh
tools/opponents/build_all.sh
build/tools/match -a build/opponents/cake/cake.dylib -b build/opponents/guinn/guinn.dylib --movetime 0.2
```

## Pondering (CheckerBoard DLL)

`set ponder 1` (`get ponder`; off by default) lets the engine library think on the opponent's time with its configured
`set threads`. After `getmove` has played, a background search starts in the position after the reply the search
expects (the second move of its PV; without one, for example after a book or database move, a short search of the
position after our move predicts the reply first). The next `getmove`:

- **hit** (the predicted position, same game): the running search gets this move's time limits, its clock starting
  now, and stops at once if it has already gone further than a normal search of this move would;
- **miss** (anything else, or `CB_RESET_MOVES`): the ponder search is stopped and a normal search runs on the warm
  hash table.

Every other library call except queries (`get ...`, `name`, ...) and `set gamehist` stops pondering first, as do
unloading the library and process exit. No pondering when the game is decided or our next move is forced; a ponder
search stops by itself after 10x the time of the last move (at least 1 s). The info string ends with
`ponder hit <ms> ms` / `ponder miss <ms> ms` (time since pondering started), and `match` prints the hit rate per
engine. Match runner: `--option-a "set ponder 1" --option-a "set threads N"`.

## Opening book and avoid list

The book file (`src/weights/book.bin`, RGBK, embedded at build time, empty by default) can also carry an
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
| `src/search.*` | Basic negamax alpha-beta with capture extension |
| `src/book.*` | Opening book and avoid list (RGBK), embedded from `src/weights/book.bin` |
| `tools/bookgen.cpp` | Opening book generator (best-first drop-out expansion) |
| `tools/lossprobe.cpp` | Lost-game analysis: blames the losing move, writes an avoid list |

Square indices are 0..31 with 0 = a1; PDN numbering (1 = b8, 32 = g1) is used for all I/O.

## Roadmap

1. Search: transposition table + Zobrist hashing, move ordering (history/killers), PVS, LMR, time control
2. Endgame databases: probe KingsRow / Cake format 8–10 piece DBs
3. NNUE: Python/PyTorch trainer (`tools/nnue/`), self-play data generation, quantized int16/int8 inference with incremental accumulator updates and SIMD (AVX2 / NEON)
4. CheckerBoard engine DLL interface for matches against Cake
5. Opening book, multi-threading (Lazy SMP)
