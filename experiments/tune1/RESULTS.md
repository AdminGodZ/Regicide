# tune1: search-parameter A/B tests

- **Bases:** series 1 on master 825f993 (NNUE nn2b, 512-wide, no embedded book, SE off; `regicide bench` = 4,518,329 nodes). Series 2 on master ae210fb (825f993 + se_depth=4; bench 6,873,528).
- **Tool:** `selfplay --a "<param=value>" --b "" --movetime 100 --threads 16 --pairs 800` (in-process, A = changed
  parameter, B = defaults). Engines use 1 search thread each, and 16 games run in parallel.
- **Openings:** the default 3-ply set (216 positions). selfplay shuffles it and cycles `i % 216`, so 800 pairs
  plays each opening ~3.7 times, each time with colours reversed.
- **No EGDB:** the laptop has no endgame database, so every match runs without `--egdb`.
- **Hardware:** i9-13900H laptop (20 threads), Arch Linux, clang 23.1.4 Release, on AC, `systemd-inhibit`, nice 10.
- Elo is A minus B, with a 95% interval from the pentanomial pair distribution (selfplay's own report).
- **Rule:** a change with ≥ +5 Elo and a CI lower bound > 0 is re-tested at 300 ms (≥ 400 pairs). Winners are then
  combined and verified against the old defaults (≥ 800 pairs at 100 ms).

✓ = at least +5 Elo with the CI lower bound above 0.

## Series 1: base 825f993 (singular extensions off, se_depth = 99)

### Single changes at 100 ms/move, 800 pairs

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_depth | 8 (99) | 336 / 968 / 296 | +8.7 [+2.0, +15.4] **✓** | 2 96 565 134 3 | 800 | 14 min |
| se_depth | 6 (99) | 369 / 929 / 302 | +14.6 [+7.7, +21.4] **✓** | 3 82 566 143 6 | 800 | 14 min |
| se_depth | 4 (99) | 376 / 945 / 279 | +21.1 [+14.3, +27.9] **✓** | 3 71 557 164 5 | 800 | 14 min |
| se_depth | 3 (99) | 359 / 975 / 266 | +20.2 [+13.7, +26.8] **✓** | 2 69 564 164 1 | 800 | 14 min |
| lmr_mult | 35 (45) | 328 / 947 / 325 | +0.7 [-6.1, +7.4] | 1 119 559 118 3 | 800 | 14 min |

### Re-tests at 300 ms/move, 400 pairs

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_depth | 8 (99) | 176 / 496 / 128 | +20.9 [+12.2, +29.5] **✓** | 1 25 301 71 2 | 400 | 20 min |
| se_depth | 6 (99) | 166 / 495 / 139 | +11.7 [+3.6, +19.9] **✓** | 1 29 313 56 1 | 400 | 20 min |
| head-to-head | se_depth=8 **vs** se_depth=6 | 136 / 521 / 143 | -3.0 [-10.7, +4.6] | 0 44 319 37 0 | 400 | 19 min |
| se_depth | 4 (99) | 168 / 495 / 137 | +13.5 [+4.9, +22.0] **✓** | 0 36 297 67 0 | 400 | 19 min |
| head-to-head | se_depth=4 **vs** se_depth=8 | 153 / 525 / 122 | +13.5 [+5.7, +21.2] **✓** | 0 27 315 58 0 | 400 | 19 min |

Outcome: `se_depth=4` was merged into master as ae210fb. The rest of series 1 (lmr_mult=35 ran on the old base) was dropped and is re-run in series 2, because LMR and pruning interact with SE.

## Series 2: base ae210fb (se_depth = 4 default, SE on in both A and B)

`regicide bench` = 6,873,528 nodes. Same tool, openings, threads and hardware as series 1. Defaults in the value column are those of ae210fb (se_depth 4).

