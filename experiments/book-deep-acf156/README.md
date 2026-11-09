# book-deep-acf156: deep opening book over all 156 ACF three-move ballots

This complements the Windows deep book (`experiments/book-deep`, 46 losing ballots). It is generated over **every**
acf156 ballot at **30M nodes per move**, the same depth, so the two can be merged by keeping the deeper entry per
position.

## Result

| | |
|---|---|
| file | `book_deep_acf156.bin`, 230,492 bytes, sha256 `802f4ad4cfe1b47a33d1812c340c1d8c8d0da366d6e743749174a20f9189cdc7` |
| book positions | **2,532** (156 roots + 2,376 expansions) |
| expansions | **2,376** = 2,160 (main run) + 216 (`--resume` top-up) |
| first root value | **−5** (ACF #1 `9-13 21-17 5-9`, White to move; book move 25-21) |
| roots with a book move | 156 / 156 (checked with `set book` + `go`: depth 0, 0 nodes) |

Root values (White to move after the ballot, `root_values.tsv`): min −49, median +9, max +84. 137 ballots are within
±29, 17 are ≥ +30 for White and 2 are ≤ −30 (#46 `9-14 23-18 14x23` −49, #153 `11-16 23-19 16x23` −42). The best
for White are #8 `9-13 22-18 11-15` +84, #16 `9-13 23-18 11-16` +72, #6 `9-13 22-18 10-14` +60 and
#10 `9-13 22-18 12-16` +54. These are 30M-node minimax values, not proven results.

## Engine and inputs

- master **ff0a3d2** (built-in NNUE n6c), clang 23.1.4 Release `-march=native`; ctest 13/13, `regicide bench` =
  8,089,571.
- `acf156_roots.txt`: the position after each of the 156 acf156 ballots (144 cross-board + 12 mail-play, barred
  ballots excluded), in ACF order. It was generated from `kAcfDeck` in `tools/match/openings.cpp` (the deck behind
  `match --openings acf156`), with each FEN from the CLI (`position startpos moves …`, `d`). All 156 FENs are distinct,
  and #1 was checked by hand.

## Commands and timing

Each run was a transient systemd user service on the i9-13900H laptop, nice 10, sleep inhibited, 18 threads:

```
bookgen --roots acf156_roots.txt --nodes 30000000 --expansions 2160 --threads 18 --out book_deep_acf156.bin
bookgen --roots acf156_roots.txt --nodes 30000000 --expansions 216  --threads 18 --out book_deep_acf156.bin \
        --resume book_deep_acf156.bin
```

| phase | start | end | duration |
|---|---|---|---|
| 156 roots analysed | 2026-11-09 04:52:05 | 05:29:33 | 37.5 min (4.2 positions/min) |
| 2,160 expansions (30 batches of 72) | 05:29:33 | 14:33:44 | 9 h 04 m (15.2-21.5 min per batch, mean 18.1; ~4.0 expansions/min) |
| `--resume` + 216 expansions | 14:33:53 | 15:27:46 | 54 min (the resume reloaded 2,316 positions in < 10 s) |
| **total** | 04:52:05 | 15:27:46 | **10 h 36 m** |

How N was chosen: each expansion searches every legal move of a position at 30M nodes. Snapshots only come every
720 expansions (~3 h), so stopping to lower N would have been costly. The run started with a conservative N = 2,160,
and once three batches had shown the real rate (~17.3 min per 72), a 216-expansion `--resume` top-up was chained to
land near 10 h.

Thermals (`monitor.log`, 670 one-minute samples): package mean 63.5 °C, max 74 °C; mean clocks 1.55 GHz P-core /
1.12 GHz E-core (power-limited by the firmware's `balanced` profile); on AC throughout.

## Files

- `book_deep_acf156.bin`: the book (RGBK, no avoid entries)
- `acf156_roots.txt`: the 156 root FENs (`bookgen --roots` input)
- `root_values.tsv`: per ballot, the ACF number, moves, kind (X/M), name, book move and value for White
- `bookgen.log`: both runs, timestamped per line
- `monitor.log`: per-minute progress, package temperature, fan speed, mean P/E clocks and AC state
