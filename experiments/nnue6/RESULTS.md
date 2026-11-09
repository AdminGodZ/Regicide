# NNUE round 6: do 50k-node labels (gen5a) beat 10k ones? (v1 FT 1024, n4c's architecture)

**Result: n6c (all data with gen5a ×3, λ 0.5) beats n4c by +4.3 Elo [+0.3, +8.4]** (95% CI, pentanomial; 1,610
games at 0.1 s/move; LOS 98.2%). It is the first net with a CI lower bound above 0 against n4c.
- The same recipe at λ 0.7 (n6b) is −3.5 [−7.3, +0.4].
- Fine-tuning n4c on gen5a alone (n6a) is −3.5 [−7.4, +0.5].
- In round 5, λ 0.5 without the deep data (l5a) gave +1.9 [−2.4, +6.3]. So n6c's gain likely comes from the
  combination: deep-label data plus less weight on the game result.

## Data

- **gen5a** (`gen5a_n50k.bin`, master 635d949, built-in n4c): **50,000 nodes per position**, 109,330 games,
  10,435,120 positions (seed 61, default random openings, 12 threads; finished 22:40).
- **The 10k-node data:** gen1, gen1b, gen1c, gen2a, gen2b, gen3a, gen3b, gen3c, gen3d, gen4a (the round-5b set, 230.6M
  training positions).
- Left out: gen4b (unfinished, paused) and gen5c (being generated). Snapshot files (`*partial*`, `gen1b_snap`) are
  not used.

## Nets

All nets are v1 1024 → 32 → 32, 8 output buckets (n4c's architecture), trained with the trainer of master b88c02e.

- **Common settings:** batch 16384, wd 0.01, 2 QAT epochs, `--compile`, `--threads 1 --loader-threads 1`.
- **Runs:**
  - one GPU lane at a time, 23:43–00:17;
  - the CPU stayed with the TM self-play series, which ran at the same time.

| Net | Start | Data | Epochs | lr | λ | Wall time |
|---|---|---|---|---|---|---|
| n6a | **n4c's round-4 checkpoint** | gen5a only | 15 × 50M | 2e-4 → 1e-5 | 0.7 | 3.5 min |
| n6b | scratch | all 10k data + gen5a ×3 | 40 × 100M | 1e-3 → 1e-5 | 0.7 | 15 min |
| n6c | scratch | all 10k data + gen5a ×3 | 40 × 100M | 1e-3 → 1e-5 | **0.5** | 15 min |

- **n6a's start:** n4c's `last.pt` with epoch and step reset to 0 and K set to 0, so it is refitted on gen5a. That way
  `--resume` loads n4c's weights and optimiser but runs a fresh 15-epoch cosine schedule with warm-up.
- **Oversampling:** gen5a ×3 means the file is given three times with `--data`. The loader maps it three times, so its
  positions are sampled three times as often, and its 2% validation tail stays held out in every copy.
- **Exports:** every net's export is exact (max |diff| 0 against `regicide evaldata` on 100k validation positions;
  `logs/export_checks.txt`).

## Losses

The scorer is `compare_loss.py` with the engine's integer eval (b88c02e). K is 106.65 (round 5b's shared config) for
every set and target. Two validation sets are used:
- **shared:** round 5b's set (v2d config), 1M positions from the 2% tails of gen1…gen4a (10k-node labels);
- **gen5a:** gen5a's held-out 2% tail, 208,702 positions with 50k-node labels. No net trained on it.

| Net | shared λ 0.7 | shared result | shared score | gen5a λ 0.7 | gen5a result | gen5a score |
|---|---|---|---|---|---|---|
| n4c | 0.506345 | 0.494304 | 0.534441 | 0.459453 | 0.440861 | 0.502834 |
| v2e (n4c arch, 10k data, 60 ep) | **0.505680** | 0.493044 | 0.535165 | 0.453514 | 0.433069 | 0.501218 |
| l5a (λ 0.5, 10k data) | 0.506489 | 0.497462 | **0.527553** | 0.455667 | 0.436511 | 0.500364 |
| n6a (n4c fine-tuned on gen5a) | 0.537897 | 0.531483 | 0.552865 | **0.439345** | **0.413276** | 0.500174 |
| n6b (+gen5a ×3, λ 0.7) | 0.506051 | 0.493529 | 0.535268 | 0.446967 | 0.424130 | 0.500255 |
| n6c (+gen5a ×3, λ 0.5) | 0.506910 | 0.497767 | 0.528244 | 0.447642 | 0.425801 | **0.498605** |

"result" is target λ 1.0 (game result only); "score" is target λ 0.0 (search score only).

- **n6a** fits gen5a best by far (0.4393), but loses badly on the 10k-label set (0.5379). Ten million positions from
  one generation pulled it away from the broader distribution, and it is −3.5 in play.
- **n6b and n6c** keep n4c-level loss on the shared set and are clearly better on the gen5a tail (0.447 vs 0.459).
- **On gen5a's search-score target**, all nets sit near 0.50. n6c is lowest (0.4986), and it is also the net that
  played best.
- As in round 5, the λ-0.7 loss does not rank the nets by playing strength. n6c is worse than v2e and n6b on the shared
  λ-0.7 loss but better in play.

## Matches vs n4c

- **Engine:** one `libregicide_cb.so` of master b88c02e for both sides (A: `set evalfile <net>`, B: the built-in n4c).
- **Settings:** `openings4.txt` 805 × 2 = 1,610 games, 0.1 s/move (CheckerBoard banked movetime), `-c 10`, hash 128,
  threads 1, book off, 2–8 piece EGDB.
- **Machine load:** datagen stopped (matchlock), no training, no other matches; 01:03–01:36.

| Match | W / D / L | Pentanomial [LL LD DD/WL WD WW] | Elo (pentanomial) [95% CI] | LOS |
|---|---|---|---|---|
| n6a vs n4c | 266 / 1062 / 282 | [0 52 717 36 0] | −3.5 [−7.4, +0.5] | 4.4% |
| n6b vs n4c | 266 / 1062 / 282 | [0 49 723 33 0] | −3.5 [−7.3, +0.4] | 3.8% |
| **n6c vs n4c** | 289 / 1052 / 269 | [0 36 713 56 0] | **+4.3 [+0.3, +8.4]** | **98.2%** |

## Next

- **Confirm n6c** at a longer time control (for example 0.3 s/move against n4c) before making it the default.
- **Round 6b:** with gen5b from the laptop (about 95k games at 50k nodes) and gen5c from this box (seed 62, running),
  repeat n6c's recipe with all deep data. Also try λ 0.5 with the deep data weighted more.

## Files

- `n6a.nnue`, `n6b.nnue`, `n6c.nnue`, `config_n6*.json`
- `logs/`:
  - training logs;
  - `losses.txt`, `export_checks.txt`;
  - the validation configs (`shared_lam*.json`, `gen5a_lam*.json`);
  - `status.txt`.
- `matches/n6*-vs-n4c/`: `summary.txt`, `results.csv`
- `scripts/`: `r6_train.sh`, `r6_match.sh`, `compare_loss.py`
