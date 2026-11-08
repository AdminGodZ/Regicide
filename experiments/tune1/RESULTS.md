# tune1: search-parameter A/B tests

- **Bases:** series 1 on master 825f993 (NNUE nn2b, SE off; bench 4,518,329). Series 2 on ae210fb (+ se_depth=4; bench 6,873,528). Series 3 on 3c87728 (NNUE n4c FT 1024, se_depth 4, se_margin 20; bench 7,338,658).
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

### Single changes at 100 ms/move, 800 pairs

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| lmr_mult | 35 (45) | 306 / 974 / 320 | -3.0 [-9.7, +3.6] | 4 115 575 103 3 | 800 | 14 min |
| lmr_mult | 55 (45) | 298 / 1006 / 296 | +0.4 [-5.9, +6.7] | 0 104 594 98 4 | 800 | 13 min |
| lmr_base | 40 (60) | 308 / 977 / 315 | -1.5 [-7.8, +4.8] | 2 106 590 101 1 | 800 | 14 min |
| lmr_base | 80 (60) | 308 / 997 / 295 | +2.8 [-3.7, +9.3] | 4 93 594 104 5 | 800 | 14 min |
| rfp_margin | 50 (70) | 308 / 993 / 299 | +2.0 [-4.5, +8.4] | 4 93 597 102 4 | 800 | 13 min |
| rfp_margin | 90 (70) | 303 / 987 / 310 | -1.5 [-8.3, +5.3] | 2 119 569 104 6 | 800 | 13 min |
| rfp_max_depth | 6 (4) | 312 / 984 / 304 | +1.7 [-4.6, +8.1] | 3 95 596 103 3 | 800 | 13 min |
| fut_margin | 40 (60) | 320 / 966 / 314 | +1.3 [-5.4, +8.0] | 3 108 573 112 4 | 800 | 13 min |
| fut_margin | 80 (60) | 290 / 999 / 311 | -4.6 [-11.0, +1.9] | 3 114 587 93 3 | 800 | 14 min |
| lmp_base | 2 (3) | 274 / 1032 / 294 | -4.3 [-10.6, +1.9] | 2 110 597 88 3 | 800 | 14 min |
| lmp_base | 5 (3) | 332 / 963 / 305 | +5.9 [-0.5, +12.2] | 2 92 584 121 1 | 800 | 14 min |
| razor_depth | 2 (0) | 295 / 1001 / 304 | -2.0 [-8.0, +4.1] | 0 106 597 97 0 | 800 | 13 min |
| razor_depth | 3 (0) | 289 / 1016 / 295 | -1.3 [-7.1, +4.5] | 2 90 621 86 1 | 800 | 13 min |
| probcut_depth | 5 (99) | 284 / 973 / 343 | -12.8 [-19.6, -6.1] | 6 138 566 89 1 | 800 | 14 min |
| se_margin | 30 (40) | 322 / 990 / 288 | +7.4 [+0.9, +13.8] **✓** | 2 89 585 121 3 | 800 | 13 min |
| se_margin | 60 (40) | 310 / 981 / 309 | +0.2 [-6.3, +6.7] | 1 112 574 111 2 | 800 | 13 min |
| se_margin | 20 (40) | 319 / 1003 / 278 | +8.9 [+2.6, +15.2] **✓** | 1 84 591 121 3 | 800 | 13 min |
| threat_qext | 2 (1) | 300 / 1009 / 291 | +2.0 [-4.1, +8.0] | 0 93 610 92 5 | 800 | 13 min |
| aspiration_delta | 8 (12) | 295 / 999 / 306 | -2.4 [-8.8, +4.0] | 4 106 587 103 0 | 800 | 13 min |
| aspiration_delta | 20 (12) | 312 / 975 / 313 | -0.2 [-6.8, +6.3] | 3 108 578 109 2 | 800 | 14 min |
| lmp_base | 8 (3) | 306 / 987 / 307 | -0.2 [-6.7, +6.2] | 2 106 586 103 3 | 800 | 14 min |

### Re-tests at 300 ms/move, 400 pairs

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_margin | 30 (40) | 139 / 527 / 134 | +2.2 [-6.0, +10.4] | 0 43 310 46 1 | 400 | 19 min |
| se_margin | 20 (40) | 153 / 519 / 128 | +10.9 [+3.1, +18.7] **✓** | 0 30 315 55 0 | 400 | 20 min |

Outcome: `se_margin=20` was merged into master (with the n4c net) as 3c87728. Series 2 ended after `lmp_base=8`; the unfinished `se_margin=10` probe moved to series 3.

## Series 3: base 3c87728 (NNUE n4c FT 1024, se_depth 4, se_margin 20)

`regicide bench` = 7,338,658 nodes. Same method as series 2. Defaults in the value column are those of 3c87728. Lazy SMP checks use fewer parallel games so that search threads x games <= 18.

### Single changes at 100 ms/move, 800 pairs

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| lmp_base | 5 (3) | 291 / 1018 / 291 | -0.0 [-5.8, +5.8] | 1 89 619 91 0 | 800 | 13 min |
| lmp_depth | 4 (3) | 284 / 1037 / 279 | +1.1 [-4.1, +6.2] | 2 65 659 74 0 | 800 | 14 min |
| se_margin | 10 (20) | 285 / 1045 / 270 | +3.3 [-2.1, +8.6] | 1 68 647 83 1 | 800 | 13 min |
| se_margin | 15 (20) | 304 / 1019 / 277 | +5.9 [-0.0, +11.8] | 1 79 613 106 1 | 800 | 14 min |
| se_depth | 3 (4) | 322 / 998 / 280 | +9.1 [+3.2, +15.0] **✓** | 1 72 612 114 1 | 800 | 14 min |
| se_depth | 5 (4) | 279 / 1009 / 312 | -7.2 [-12.9, -1.4] | 3 98 629 69 1 | 800 | 14 min |
| single_reply_ext | 0 (1) | 266 / 1024 / 310 | -9.6 [-15.1, -4.0] | 1 105 631 63 0 | 800 | 14 min |
| single_reply_ext | 2 (1) | 283 / 1033 / 284 | -0.2 [-5.6, +5.1] | 1 75 650 72 2 | 800 | 13 min |
| lmr_min_depth | 2 (3) | 291 / 1026 / 283 | +1.7 [-3.9, +7.4] | 0 83 626 91 0 | 800 | 14 min |

### Re-tests at 300 ms/move, 400 pairs

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_depth | 3 (4) | 141 / 535 / 124 | +7.4 [-0.1, +14.8] | 0 29 326 44 1 | 400 | 19 min |
| se_depth | 3 (4) | 268 / 1074 / 258 | +2.2 [-2.6, +6.9] | 0 57 676 67 0 | 800 | 38 min |

Note: on 2026-11-08 a T3 app restart killed the runner during series-3 `lmr_min_depth=2` (1160/1600 games, discarded). From 14:43 the runner has run as a systemd user service, and `lmr_min_depth=2` was re-run from scratch.

**Series 3 paused** on 2026-11-08 at 15:36, after the `se_depth=3` 800-pair re-test at 300 ms, for the avoid-list job (t3/avoid-v1). Still queued in `~/regicide-jobs/tune1/queue.txt`: lmr_min_depth=4, etc_min_depth 3/6, cont_hist=0, aspiration_delta=20, and the Lazy SMP checks 2v1 and 4v2.

`se_depth=3`: +9.1 [+3.2, +15.0] at 100 ms, but +7.4 [-0.1, +14.8] (400 pairs) and +2.2 [-2.6, +6.9] (800 pairs) at 300 ms. Not confirmed; se_depth stays 4.

