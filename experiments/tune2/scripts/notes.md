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
