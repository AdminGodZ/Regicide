# book_w03: opening book regenerated with the w_lr0.03_wd0 eval

`book_w03.bin` regenerates the 40k-expansion opening book with the built-in eval of a44591e
(`w_lr0.03_wd0`, +163.7 Elo vs the eval that produced `book_b13`). Generated on the Arch laptop.

## Hardware

- Intel Core i9-13900H (6 P-cores with HT + 8 E-cores = 20 threads), 62 GiB RAM, Dell laptop, on AC throughout
- Arch Linux, kernel 7.2.14; intel_pstate `powersave` governor, EPP `balance_performance`, ACPI platform profile `balanced`
- Built with clang 23.1.4, Release (`-O3 -march=native`). GCC 16 can't build the tree (`__builtin_bitreverse32` in eval.cpp is clang-only).
- `regicide bench 15`: 2,058,069 nodes (same as the Mac), 5.67-5.72M nps single thread (3 runs)

## Command

```
systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide bookgen" nice -n 5 \
  bookgen-a44591e --nodes 2000000 --expansions 40000 --threads 18 --out ~/regicide-data/book/book_w03.bin
```

The binary was a pinned copy of `build-arch/bookgen` at a44591e (no `--evalfile`; built-in eval). The effective nice
level was 10 because the launching shell was already at nice 5. The run was not interrupted and needed no resume.

## Duration and rate

- Start 2026-11-07 23:01:24 +01:00, end 2026-11-08 03:56:35 +01:00: **4 h 55 min** wall time
- **~136 expansions/min** (40,032 expansions / 295 min), steady from start to finish (about 133/min in the first 10 min, 135-140/min later)
- For comparison, the Ultra 9 285K (Windows, 10 threads) took 3 h 25 min for 40k, about 195/min, so this laptop is ~0.7x.
- Thermals (`monitor.log`, one sample per minute): package mean 63 °C, max 83 °C; fans ~2,100-2,400 RPM.
  Mean clocks under load were only ~1.8 GHz P-core and ~1.3 GHz E-core. The package is power-limited by the firmware's
  `balanced` profile, not thermally throttled. The `performance` platform profile (needs root) would probably be faster.

## Final book

- Expansions: 40,032 (batches of 72 = 18 threads x 4)
- Book positions: **40,249** (217 root positions after 3 plies + expansions)
- Start-position value: **+7** (it was -2 after the roots and settled between +3 and +7 for the second half)
- Final frontier priority: 143
- File: 3,975,804 bytes, sha256 `1121af3b4f51d7795f9e361e9f351369c9cf18a1a7209dc32563c0618c811e97`

Log tail:

```
expanded 39888  book positions 40105  next priority 140  start value 7
expanded 39960  book positions 40177  next priority 142  start value 7
expanded 40032  book positions 40249  next priority 143  start value 7
wrote /home/admingod/regicide-data/book/book_w03.bin with 40249 positions
END 2026-11-08T03:56:35+01:00 rc=0
```

## Files

- `book_w03.bin`: the book (same format as `src/weights/book.bin`; not embedded by this commit)
- `bookgen.log`: full bookgen output, with START/END timestamps from the wrapper script
- `monitor.log`: per-minute progress, package temperature, fan speed, mean P/E-core MHz and AC state
