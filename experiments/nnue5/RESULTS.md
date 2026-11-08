# NNUE round 5: v2 architecture (both views into L1, bucketed hidden layers) vs n4c

**Result: no v2 net beats n4c in play, although all of them have clearly lower validation loss.** All results are vs
n4c, 95% CI, pentanomial, 1,610 games at 0.1 s/move:
- v2b (2×1024, L1 16): +0.6 [−4.0, +5.3];
- v2a (2×512): −1.3 [−5.5, +2.9];
- round 5b's best by loss, v2d (2×512, 80 epochs on all data): −0.2 [−4.7, +4.2].

Validation-loss gains of 0.002–0.004 over n4c gave about 0 Elo at this time control. The ablation v2c (1 bucket) is no
better than n4c. v2e, n4c's arch retrained on more data, shows that data alone is worth about 0.0007.

## Setup

- **Code:** `t3/nnue-v2` @ `b05f80a`, in a worktree `~/regicide-v2`, Release, clang, `-march=native` (AVX-VNNI).
- **Verification**, the first run of the v2 code on x86 hardware (`logs/build_and_ctest.txt`):
  - ctest **12/12** passes, including `nnue_v2`, `nnue_v2_wide` and `nnue_v2_shapes`, which check the AVX-VNNI kernel
    against the reference.
  - bench **7,338,658** (built-in n4c).
  - Random-net `check_export` (v1, v2, v2 2×1024/L1 16): max |diff| 0.
- **Data** (`logs/data_files.txt`):
  - gen1, gen1b, gen1c, gen2a, gen2b, gen3a, gen3b, gen3c, gen3d (complete);
  - plus a whole-record prefix of gen4a (`gen4a_partial_19267072.bin`, 19,267,072 records, taken at 13:07 while gen4a
    was being generated).
  - That gives 259,536,901 training and 1,000,000 validation positions.
  - `gen1b_snap.bin`, a prefix of gen1b, is left out. `train_v2.sh`'s `*.bin` glob would have picked it up, so the
    runs use a directory of symlinks (`DATA_DIR`).
- **Training:** `experiments/nnue_v2/scripts/train_v2.sh` with the round-4 settings: 40 × 100M, batch 16384, lr 1e-3 →
  1e-5, wd 0.01, λ 0.7, K fitted (107.95), 2 QAT epochs, `--compile`, `--threads 1 --loader-threads 1`.
  - Two GPU lanes on the RTX 5080 at about 2.4M pos/s each.
  - Wall times: v2a 13:07–13:35, v2b 13:07–13:36, v2c 13:35–13:51 (4.2M pos/s alone).

| Net | Arch | Options |
|---|---|---|
| v2a | v2 2×512 → 32 → 32, 8 buckets | `--arch v2 --ft 512 --l1 32 --l2 32 --buckets 8` |
| v2b | v2 2×1024 → 16 → 32, 8 buckets | `--arch v2 --ft 1024 --l1 16 --l2 32 --buckets 8` |
| v2c | v2 2×512 → 32 → 32, 1 bucket (ablation) | `--arch v2 --ft 512 --buckets 1` |

**Export checks** (`logs/export_checks.txt`): for all three nets, the Python integer model equals `regicide evaldata`
exactly (max |diff| 0) on three sets: 100k positions of the last data file, the 200k gen2b slice, and 100k validation
positions. Each `.nnue` file is byte-identical to the final epoch's export.

## Loss on one shared validation set

The validation set is v2a's: the 2% tail of each data file, 1M positions, seed 1, λ 0.7, K 107.95. Every net is scored
with the engine's integer eval from the one t3/nnue-v2 binary (`scripts/compare_loss.py`). n4c never trained on any of
these positions: its gen3d data stopped at record 18.37M, and the gen3d tail starts at 28.0M.

| Net | Loss | vs n4c |
|---|---|---|
| n4c (v1 1024, built-in) | 0.508497 | — |
| v2a (2×512, 8 buckets) | 0.506098 | −0.002399 |
| **v2b (2×1024, L1 16, 8 buckets)** | **0.504773** | **−0.003724** |
| v2c (2×512, 1 bucket) | 0.508883 | +0.000386 |

- **The bucketed hidden layers carry the gain.** With both views but a single bucket, v2c is no better than n4c.
- Part of each v2 net's lead is data: they trained on the full gen3d plus 19M gen4a records that n4c never saw. In
  round 4, more data at a fixed arch was worth about 0.0012. v2e, an n4c retrain on all the data, measures this
  directly.

## Speed (bench, one t3/nnue-v2 build)

Three alternating rounds (`logs/bench.txt`). Datagen was stopped and no matches ran. The round-5b trainer (v2d, about
1.2 cores) was running.

| Net | Bench nodes | Mean nps | vs n4c |
|---|---|---|---|
| n4c (built-in) | 7,338,658 | 3,307,786 | 100% |
| v2a | 6,616,790 | 3,341,371 | 101.0% |
| v2b | 6,769,548 | 2,786,220 | 84.2% |
| v2c | 6,934,028 | 3,474,155 | 105.0% |

As predicted from the M5 numbers, v2a runs at n4c's speed. v2b costs 16%, less than the 22% the M5 showed.

## Matches vs n4c

The two lowest-loss nets, v2b and v2a, each played n4c:
- **Engine:** one `libregicide_cb.so` from t3/nnue-v2 for both sides. A uses `set evalfile <net>`; B uses the built-in
  n4c.
- **Settings:** `openings4.txt` (805 openings × 2 = 1,610 games), 0.1 s/move, `-c 10`, hash 128, threads 1, book off,
  2–7 piece EGDB, `tools/match` + `cbhost` from the same build.
- **Machine load:**
  - gen4b datagen was stopped (state `T`, together with its bash parent).
  - The round-5b GPU trainer ran (about 1.2 cores).
  - From 14:27, the 8-piece EGDB build ran at 8 threads (total about 19 of 24 cores). Both sides were equally affected.

| Match | W / D / L | Pentanomial [LL LD DD/WL WD WW] | Elo (pentanomial) [95% CI] | LOS |
|---|---|---|---|---|
| v2b vs n4c | 287 / 1039 / 284 | [0 58 686 61 0] | **+0.6 [−4.0, +5.3]** | 60.8% |
| v2a vs n4c | 277 / 1050 / 283 | [0 53 705 47 0] | **−1.3 [−5.5, +2.9]** | 27.4% |

Run times: v2b 14:20–14:35, v2a 14:35–14:49.

**Reading:** the loss gains of 0.0024 and 0.0037 did not turn into Elo at 0.1 s/move. In round 4, n4c's 0.0020 over nn3e
was worth +5.6. Two caveats:
- The validation set may favour the v2 nets because of their extra data.
- v2b pays 16% nps.

Neither net is a round-5 winner (CI lower bound > 0), so the round-5b match is against n4c.

## Round 5b: longer training, an n4c retrain, 2×768

- **Data:** all of it: gen1…gen3d plus gen4a complete (28,083,233 positions). `logs/data2_files.txt`.
- **Training:** the same settings (`scripts/r5b_chain.sh`).
  - v2d started at 13:52 on one lane. The second lane started only after the round-5 matches, so at most one trainer
    ran during matches.
  - Wall times: v2d 13:52–14:32 (80 epochs), v2e 14:32–14:59 (60 epochs), v2f 14:49–15:10.
- **Exports:** every 5b net's export is exact (max |diff| 0) on 100k positions of the last data file, the 200k gen2b
  slice and 100k validation positions (`logs/export_checks_5b.txt`).

| Net | Arch | Epochs |
|---|---|---|
| v2d | v2 2×512 → 32 → 32, 8 buckets | 80 |
| v2e | **v1** 1024 → 32 → 32 (n4c's arch, retrained on all data) | 60 |
| v2f | v2 2×768 → 16 → 32, 8 buckets | 40 |

### Loss on v2d's validation set

This set includes the tail of the complete gen4a, so it is new to every net. λ 0.7, K 106.65, one t3/nnue-v2 binary
(`logs/losses_5b.txt`).

| Net | Loss | vs n4c |
|---|---|---|
| n4c | 0.506345 | — |
| v2c (2×512, 1 bucket) | 0.506604 | +0.000259 |
| **v2e (n4c arch, all data, 60 ep)** | 0.505680 | **−0.000665** |
| v2a (2×512, 40 ep, round-5 data) | 0.503762 | −0.002583 |
| v2f (2×768, L1 16) | 0.503292 | −0.003053 |
| **v2d (2×512, 80 ep, all data)** | 0.503256 | −0.003089 |
| v2b (2×1024, L1 16, round-5 data) | 0.502362 | −0.003983 |

- **Data alone is worth about 0.0007** (v2e vs n4c, same arch).
- **The v2 bucketed layers are worth about 0.0025–0.0033 more.** Doubling the epochs (v2d vs v2a) adds 0.0005.
- v2b is still the lowest-loss net, but it is a round-5 net and was already matched (+0.6). The best 5b net, v2d, was
  matched.

### Speed (one t3/nnue-v2 build, `logs/bench_5b.txt`)

Three rounds, run while the 8-piece EGDB build was running at 8 threads (bookgen and datagen paused, no matches).

| Net | Bench nodes | Mean nps | vs n4c |
|---|---|---|---|
| n4c (built-in) | 7,338,658 | 3,322,694 | 100% |
| v2d | 7,929,421 | 3,318,499 | 99.9% |
| v2e | 6,887,518 | 3,439,795 | 103.5% |
| v2f | 8,203,054 | 3,198,977 | 96.3% |

### Match: v2d vs n4c

The match used the same settings as round 5: one `rv2.so`, `set evalfile v2d.nnue` vs the built-in n4c, 1,610
games, 0.1 s/move, `-c 10`, book off, 2–7 EGDB. It ran 15:12–15:26.
- **Machine load:**
  - the EGDB build at 8 threads;
  - the v2g trainer from 15:16 (about 1.2 cores);
  - gen4b datagen and bookgen were stopped.
- No round-5 net had a CI lower bound > 0, so the opponent was n4c.

| Match | W / D / L | Pentanomial | Elo (pentanomial) [95% CI] | LOS |
|---|---|---|---|---|
| v2d vs n4c | 273 / 1063 / 274 | [0 56 694 55 0] | **−0.2 [−4.7, +4.2]** | 46.2% |

### v2g

v2g is v2b's architecture (2×1024, L1 16) trained on all data for 40 epochs (`scripts/v2g.sh`, 15:16–15:39), run to
keep the GPU busy.
- Its export check is exact (`logs/export_checks_v2g.txt`).
- Loss on v2d's validation set: **0.502306**, the lowest of all nets. v2b scored 0.502362 and n4c 0.506345.
- Not matched: v2b, the same architecture with slightly less data, was +0.6 [−4.0, +5.3].

## λ sweep (v1 FT 1024, all data)

n4c's architecture (v1 1024 → 32 → 32, 8 output buckets), trained on all data (the round-5b set) for 40 epochs with
n4c's settings except `--lambda` (`scripts/r6_lambda.sh`). One GPU lane, 15:43–16:28.

| Net | λ (weight of the game result in the target) |
|---|---|
| l5a | 0.5 |
| v2e (round 5b, 60 epochs) | 0.7 (n4c's) |
| l5b | 0.9 |
| l5c | 1.0 (game result only) |

All exports are exact (`logs/export_checks_l5.txt`). Loss on v2d's validation set (K 106.65, one t3/nnue-v2 binary) is
given for three targets, because a loss is only comparable at a fixed target λ:

| Net | Target λ 0.7 | Target λ 1.0 (result only) | Target λ 0.0 (search score only) |
|---|---|---|---|
| n4c | 0.506345 | 0.494304 | 0.534441 |
| v2e (λ 0.7) | **0.505680** | 0.493044 | 0.535165 |
| l5a (λ 0.5) | 0.506489 | 0.497462 | **0.527553** |
| l5b (λ 0.9) | 0.507260 | 0.490490 | 0.546390 |
| l5c (λ 1.0) | 0.509768 | **0.490192** | 0.555447 |

Each net is best on the target nearest its own λ, as expected, so the losses cannot pick a λ. The matches decide. They
used the round-5 settings (one `rv2.so`, `set evalfile` vs the built-in n4c, 1,610 games, 0.1 s/move, `-c 10`, book
off, gen5 datagen and bookgen stopped):

| Match | W / D / L | Pentanomial | Elo (pentanomial) [95% CI] | LOS |
|---|---|---|---|---|
| l5a (λ 0.5) vs n4c | 284 / 1051 / 275 | [0 48 700 57 0] | +1.9 [−2.4, +6.3] | 81.0% |
| l5b (λ 0.9) vs n4c | 267 / 1057 / 286 | [0 51 722 32 0] | **−4.1 [−7.9, −0.3]** | 1.8% |

- **More weight on the game result (λ 0.9) is worse.** Its CI is entirely below 0.
- **Less weight (λ 0.5) is not significantly better** than n4c. Its point estimate (+1.9) is above l5b's by 6 Elo.
- l5c (λ 1.0) was not matched; l5b already shows the direction.
- If anything, λ ≤ 0.7 is right; λ 0.5 could be tried against v2e (the same data at λ 0.7) in a longer match.
- **Run times:** l5a 16:30–16:44; l5b 17:32–17:43, after the 8-piece baseline.
- **Machine load during l5b:** from about 17:36 to 17:42, ad-hoc engine speed tests (1 to 8 extra `regicide`
  processes) ran alongside, for the 8-piece EGDB analysis. The total stayed under 24 busy cores, and both sides were
  affected equally.

### Conclusion

At 0.1 s/move, the v2 nets' 0.0025–0.004 lower validation loss is worth about 0 Elo against n4c. In round 4, a 0.0020
gain (n4c over nn3e) was worth +5.6. Possible reasons:
- The loss gain may sit in positions that matter little in play.
- The bucketed nets may need different λ/K.
- The time control may be too short to show it.

A longer-time-control match (for example v2b or v2d at 0.3 s) would be the next check before giving up on v2.

## Files

- `v2a.nnue`, `v2b.nnue`, `v2c.nnue` (later also v2d–v2f), `config_*.json`
- `logs/`:
  - per net: training log, `*.check.txt`, `*.train_bench.txt`, `*.data_args.txt`;
  - `export_checks.txt`, `losses.txt`, `bench.txt`, `data_files.txt`;
  - `build_and_ctest.txt`, `status.txt`, `jobs_status.txt`.
- `matches/<net>-vs-n4c/`: `summary.txt`, `results.csv`
- `scripts/`: `r5_build.sh`, `r5_chain.sh`, `gate_2s.sh`, `compare_loss.py`, `pausekeeper.sh`, `launch.sh`
  - `gate_2s.sh` held the matches until the 2 s KingsRow baseline ended. The chain's own check had matched a stale
    log line.
