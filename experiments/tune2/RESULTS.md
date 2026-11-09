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
| rfp_margin | 50 (70) | 252 / 1095 / 253 | -0.2 [-4.9, +4.5] | 1 58 683 57 1 | 800 | 38 min |
| etc_min_depth | 6 (4) | 258 / 1068 / 274 | -3.5 [-8.2, +1.3] | 0 70 676 54 0 | 800 | 39 min |
| cont_hist | 0 (1) | 270 / 1072 / 258 | +2.6 [-2.2, +7.4] | 0 58 672 70 0 | 800 | 38 min |
| aspiration_delta | 20 (12) | 258 / 1079 / 263 | -1.1 [-6.0, +3.8] | 0 69 667 64 0 | 800 | 38 min |

## Outcome (2026-11-09, 20:41)

All 8 screens finished (6 candidates + 2 extras, 300 ms × 800 pairs each, 15:27-20:41). **No candidate reached the
confirmation bar** (≥ +5 Elo with the CI lower bound > 0), so no 1000 ms confirmation was run, and **the defaults of
6bb8c6c stay unchanged**. With the n6c net, tune1's near-misses are all neutral or worse:

- `se_margin` 15 −1.7, 30 −6.5 (significantly worse): 20 stays.
- `se_depth=3` −2.2: 4 stays (consistent with tune1's 300 ms results, +7.4 and +2.2).
- `lmr_base=80` +3.7 [−1.1, +8.4]: the best of the set but below the bar. It looked like +8.7 after 680 games and
  faded to +3.7. This is a candidate for a longer run (e.g. 1600 pairs) if search tuning resumes.
- `rfp_margin=50` −0.2, `etc_min_depth=6` −3.5, `aspiration_delta=20` −1.1: neutral.
- `cont_hist=0` +2.6 [−2.2, +7.4]: neutral at 300 ms here and at 100 ms in tune1 (+0.2). Continuation history could be
  removed as a simplification, but it is not an Elo gain.

Unfinished tests: none. The runner exited normally after the last test (`STOP` file); nothing is queued.
