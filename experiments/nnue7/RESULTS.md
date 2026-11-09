# NNUE round 7 (vs n6c): training and validation losses only, not matched yet

**Status:** trained and scored on 2026-11-09. **No matches were played** before the 22:00 deadline, so there is no
strength result yet. The match setup is ready (`scripts/r7_match.sh`, see "Next").

## Nets

All nets are v1 1024 → 32 → 32, 8 output buckets (n6c's architecture), trained with the trainer of master b88c02e
(as rounds 6/6b). Each run used one GPU lane capped at `--threads 1 --loader-threads 1`.

- **n6c's recipe:** all 10k data (gen1…gen4a) + gen5a ×3, 40 × 100M, lr 1e-3 → 1e-5, λ 0.5.
- **n6d (round 6b) is reused as the λ 0.3 candidate.** It is that recipe at λ 0.3, the run asked for here as "n7a",
  so the new n7a probes λ 0.4.

| Net | Change from n6c | Epochs | Trained |
|---|---|---|---|
| n6d (round 6b) | λ 0.3 | 40 | 2026-11-09 02:14–02:25 |
| **n7a** | λ **0.4** | 40 | 12:21–12:47 |
| **n7b** | + gen5c (50k nodes) whole-record prefix `gen5c_partial_9472000.bin` ×3 | **60** (1.5×) | 12:47–13:12 |

gen5c finished later that day (10,399,320 records). n7b used its first 9,472,000.

**Exports:** both n7 exports are exact (max |diff| 0 against `regicide evaldata`, `logs/export_checks.txt`).

## Losses

The set-up is round 6's: the shared set (1M, 10k labels) and gen5a's held-out tail (208,702, 50k labels), K 106.65,
engine integer eval. Source: `logs/losses.txt`.

| Net | shared λ 0.7 | shared λ 0.5 | shared result | shared score | gen5a λ 0.7 | gen5a λ 0.5 | gen5a result | gen5a score |
|---|---|---|---|---|---|---|---|---|
| n4c | 0.506345 | 0.514372 | 0.494304 | 0.534441 | 0.459453 | 0.471847 | 0.440861 | 0.502834 |
| n6c | 0.506910 | 0.513005 | 0.497767 | 0.528244 | 0.447642 | 0.462203 | 0.425801 | 0.498605 |
| n6d (λ 0.3) | 0.509411 | 0.513461 | 0.503336 | **0.523585** | 0.453032 | 0.466041 | 0.433518 | 0.498565 |
| n7a (λ 0.4) | 0.507779 | **0.512826** | 0.500209 | 0.525442 | 0.450656 | 0.464256 | 0.430255 | 0.498258 |
| n7b (+gen5c, 60 ep) | **0.506722** | 0.512936 | **0.497400** | 0.528472 | **0.446558** | **0.461276** | **0.424482** | **0.498071** |

- **n7b is the best fit to the deep labels** (gen5a tail) on every target, ahead of n6c by about 0.001. It is about
  equal to n6c on the shared set.
- **n7a and n6d** trade fit on the game result for fit on the search score, as their λ predicts. Round 6 showed that
  these losses do not rank nets by playing strength (n6c won its match despite a worse shared λ-0.7 loss than n6b), so
  only matches can decide.

## Next

1. **Matches vs n6c**, n6d, n7a and n7b, with `scripts/r7_match.sh`:
   - one `libregicide_cb.so` of master ff0a3d2 (built-in n6c) for both sides, A with `set evalfile`;
   - 0.1 s/move, `openings4.txt` 805 × 2, `-c 10`, book off, 2–8 EGDB, datagen paused;
   - about 12 min each.
   - The script waits for `D2 D2_ALL_DONE` and `R7 R7_SCORED` in `~/regicide-jobs/r4/status`. Remove or satisfy those
     lines to run it.
2. **If n7b wins,** a round with all three 50k files (gen5a, gen5b from the laptop, gen5c complete) is the natural
   follow-up.

## Files

- `n7a.nnue`, `n7b.nnue`, `config_n7a.json`, `config_n7b.json`
- `logs/`: `n7a.log`, `n7b.log`, `losses.txt`, `export_checks.txt`, `status.txt`
- `scripts/`: `r7_train.sh`, `r7_match.sh`
