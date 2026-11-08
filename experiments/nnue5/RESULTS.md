# NNUE round 5: v2 architecture (both views into L1, bucketed hidden layers) vs n4c

**Result so far: neither v2 net beats n4c in play, although both have clearly lower validation loss.**
v2b (2×1024, L1 16) is +0.6 [−4.0, +5.3] and v2a (2×512) is −1.3 [−5.5, +2.9] vs n4c (95% CI, pentanomial, 1,610
games at 0.1 s/move). The round-5b batch (v2d: v2a for 80 epochs; v2e: v1 FT 1024 for 60 epochs, an n4c retrain on more
data; v2f: 2×768, L1 16) is still training. Its results will be added here.

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

## Round 5b (in progress)

- **v2d:** v2 2×512, 8 buckets, 80 epochs. Finished at 14:32; its export check is exact.
- **v2e:** v1 FT 1024, 60 epochs. Training.
- **v2f:** v2 2×768, L1 16, 8 buckets. Training.
- **Data:** all of it, including gen4a complete (28,083,233 positions).
- **Plan:** the losses of every round-5/5b net and n4c on v2d's validation set, the nps bench, then the best 5b net vs
  n4c under the same match settings.

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
