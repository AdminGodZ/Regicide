# tune1: search-parameter A/B tests

- **Base:** master 825f993 (NNUE nn2b, 512-wide, no embedded book). `regicide bench` = 4,518,329 nodes.
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

## Single changes at 100 ms/move

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_depth | 8 (99) | 336 / 968 / 296 | +8.7 [+2.0, +15.4] **✓** | 2 96 565 134 3 | 800 | 14 min |
| se_depth | 6 (99) | 369 / 929 / 302 | +14.6 [+7.7, +21.4] **✓** | 3 82 566 143 6 | 800 | 14 min |
| se_depth | 4 (99) | 376 / 945 / 279 | +21.1 [+14.3, +27.9] **✓** | 3 71 557 164 5 | 800 | 14 min |

## Re-tests at 300 ms/move (400 pairs)

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_depth | 8 (99) | 176 / 496 / 128 | +20.9 [+12.2, +29.5] **✓** | 1 25 301 71 2 | 400 | 20 min |
| se_depth | 6 (99) | 166 / 495 / 139 | +11.7 [+3.6, +19.9] **✓** | 1 29 313 56 1 | 400 | 20 min |

