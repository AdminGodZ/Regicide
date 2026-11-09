# tune2: search-parameter A/B tests on the n6c engine

- **Base:** master 6bb8c6c (built-in NNUE n6c, embedded deep book; se_depth 4, se_margin 20). `regicide bench` =
  7,712,020 with the book on and 8,089,571 with `set book off` (= ff0a3d2). ctest 13/13.
- **Tool:** `selfplay --a "<param=value>" --b "" --movetime 300 --threads 16 --pairs 800` (in-process; A = changed
  parameter, B = defaults; 1 search thread per engine, 16 games in parallel). selfplay never loads the embedded book
  (only the CLI and the CheckerBoard DLL call `set_book`), so the book is off in both engines. Avoid lists are off
  (selfplay's default).
- **Openings:** the default 3-ply set (216 positions), shuffled and cycled, each played twice with colours reversed,
  as in tune1.
- **No EGDB** in the matches (as in tune1).
- **Hardware:** i9-13900H laptop, Arch Linux, clang 23.1.4 Release; a systemd user service, nice 10, sleep inhibited.
- **Method:** screen at **300 ms, 800 pairs**. A candidate with ≥ +5 Elo and a 95% CI lower bound > 0 is re-tested at a
  longer time control (**1000 ms, 400 pairs**) and counts as a winner only if its lower bound is > 0 there too.
- **Candidates:** tune1's most promising near-misses, re-checked with the n6c net: se_margin 15 (+5.9 [−0.0, +11.8] in
  tune1 s3) and 30, se_depth 3 (+9.1 at 100 ms but not confirmed at 300 ms), lmr_base 80 (best LMR variant, +2.8),
  rfp_margin 50 (best RFP/futility variant, +2.0) and etc_min_depth 6 (+4.6 [−1.3, +10.5]). Aspiration 8/20 was left
  out, because it was flat three times in tune1 (−2.4, −0.2, −3.0).

✓ = at least +5 Elo with the CI lower bound above 0.

## Results

### Screen at 300 ms/move

| param | value (default) | W / D / L | Elo [95% CI] | penta [LL LD DD/WL WD WW] | pairs | time |
|---|---|---|---|---|---|---|
| se_margin | 15 (20) | 270 / 1052 / 278 | -1.7 [-6.7, +3.2] | 0 71 666 63 0 | 800 | 39 min |
| se_margin | 30 (20) | 255 / 1060 / 285 | -6.5 [-11.5, -1.5] | 0 84 662 54 0 | 800 | 39 min |
| se_depth | 3 (4) | 256 / 1078 / 266 | -2.2 [-7.2, +2.9] | 0 76 658 66 0 | 800 | 38 min |
| lmr_base | 80 (60) | 272 / 1073 / 255 | +3.7 [-1.1, +8.4] | 0 54 675 71 0 | 800 | 38 min |

