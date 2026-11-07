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

## Layout

| File | Purpose |
|---|---|
| `src/types.hpp` | 32-bit bitboard, diagonal shifts, PDN square mapping |
| `src/position.*` | Board state, PDN FEN, make move |
| `src/movegen.*` | Legal moves: compulsory captures, multi-jumps, crowning ends the move |
| `src/perft.*` | Node counting for move generation tests |
| `src/eval.*` | Hand-crafted placeholder eval (NNUE goes here) |
| `src/search.*` | Basic negamax alpha-beta with capture extension |

Square indices are 0..31 with 0 = a1; PDN numbering (1 = b8, 32 = g1) is used for all I/O.

## Roadmap

1. Search: transposition table + Zobrist hashing, move ordering (history/killers), PVS, LMR, time control
2. Endgame databases: probe KingsRow / Cake format 8–10 piece DBs
3. NNUE: Python/PyTorch trainer (`training/`), self-play data generation, quantized int16/int8 inference with incremental accumulator updates and SIMD (AVX2 / NEON)
4. CheckerBoard engine DLL interface for matches against Cake
5. Opening book, multi-threading (Lazy SMP)
