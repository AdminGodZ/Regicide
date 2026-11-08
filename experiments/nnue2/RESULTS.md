# nnue2: NNUE round 2 (nn2a, nn2b, nn2c) vs nn1

All runs used `master` @ `747ab51`. Its built-in net is nn1 (`experiments/nnue1/nn1.nnue`), with the linear G2c as
fallback. nn2b needs a feature transformer of 512 instead of 256, so it was trained and played with a scratch
worktree of 747ab51 that has two one-line changes:
- `constexpr int FT = 512;` in `src/nnue.hpp`;
- `INPUTS, FT, ... = 128, 512, ...` in `tools/nnue/nnue.py`.

In that worktree `src/weights/net.bin` is empty, because the embedded 256-wide nn1 does not fit the 512 architecture.
The scratch build passes ctest. Its bench with the linear fallback is 4,008,356 nodes (the same as the G2c built-in),
and `check_export.py` with a random 512 net is exact. Hardware and software: RTX 5080, PyTorch 2.11.0+cu128,
WSL2, clang 22.

## Data

gen1, gen1b, gen1c, gen2a, gen2b (as nn1) plus **gen3a**. gen3a was still being generated when training started.
The runs therefore used a snapshot, `gen3a_partial_11831552.bin`: the first 11,831,552 whole records of
`gen3a_n10k.bin`, taken at 01:51 (about 112k games). The finished gen3a has 30,033,050 positions (285,457 games).
- Train: 137,872,244 positions.
- Validation: 1,000,000 positions sampled (seed 1) from the last 2% of each of the 6 files.
- Fitted K: 118.6 (the same for all 3 runs, which share the data and seed).

## Training

These are the nn1 settings (`--epoch-size 100000000 --batch 16384 --lr 1e-3 --lr-final 1e-5 --wd 0.01 --qat-epochs 2
--compile`). Each run changes only the following:

| Run | Change vs nn1 | Epochs | Time |
|---|---|---|---|
| nn2a | none (same arch) | 100 | 01:52 to 02:34 |
| nn2b | FT 512 | 40 | 02:34 to 02:51 |
| nn2c | `--lambda 0.5` | 60 | 02:51 to 03:13 |

The runs went one after another, on the GPU, while the 747ab51 baseline ran (`results/baseline-747ab51`). To keep the
CPU load comparable to the earlier baselines (<= 22 busy threads), they used `OMP_NUM_THREADS=1 --threads 1
--loader-threads 1`, which takes about 1.2 cores and gives 3.7-4.8M pos/s. nn1 used the defaults, about 10 cores and
4.8M pos/s. The logs are in `logs/`.

Valid loss from the training logs. Each run measures it on its own validation set, the same set for all three. nn2c's
column uses lambda 0.5, so it is not comparable with the others.

| Epoch | nn2a valid(int) | nn2b valid(int) | nn2c valid(int) (lambda 0.5) |
|---|---|---|---|
| 1   | 0.539266 | 0.537984 | 0.546545 |
| 10  | 0.536794 | 0.534597 | 0.544138 |
| 20  | 0.535946 | 0.534020 | 0.543936 |
| 30  | 0.535903 | 0.533854 | 0.543767 |
| 40  | 0.536132 | **0.533150** (final) | 0.544318 |
| 60  | 0.535307 | | **0.543358** (final) |
| 100 | **0.535011** (final) | | |

### Comparison on one validation set

Every net was scored on the same 1M validation positions, with lambda 0.7 and K 118.57 (nn2a's config;
`compare_loss.py`). Scores come from `regicide evaldata`, the engine's integer eval. nn2b was scored by the FT-512 build.
nn1 never trained on any of these positions: they are the held-out tails of the same 5 files, plus gen3a, which nn1
never saw.

| Net | Valid loss (lambda 0.7, K 118.57) |
|---|---|
| **nn2b** (FT 512, 40 ep) | **0.533141** |
| **nn2a** (100 ep) | **0.535018** |
| nn1 (built-in) | 0.535867 |
| nn2c (lambda 0.5, 60 ep) | 0.536344 |
| G2c (linear) | 0.543507 |

- The best two by this loss are **nn2b** and **nn2a**, and both were matched against nn1.
- nn2c is last among the nets. It was trained toward a different target (lambda 0.5), so a loss at lambda 0.7 is not
  a fair test of its strength, but the rule was to pick by this loss.
- Doubling the FT width helps far more than training 2.5x longer: nn2b is 0.0019 below nn2a, and nn2a is only 0.0008
  below nn1.

## Export checks (must be exact)

For each run, `check_export.py --checkpoint last.pt --net <run>.nnue` was run with the matching build: the 747ab51
build for nn2a and nn2c, the FT-512 build for nn2b. Each exported net is byte-identical to the trainer's final
`net_eNNN.nnue`. Python int vs engine was checked on 1000 positions of `gen2b_n10k.bin` and on 100,000 validation
positions: max |diff| 0 in every case. **OK** (`logs/export_checks.txt`).

## Bench (single thread)

| Net | bench nodes | nps, 3 runs (M) | median |
|---|---|---|---|
| nn1 (built-in, FT 256) | 4,082,499 | 4.77 / 4.81 / 5.44 | 4.81M |
| nn2a (`set evalfile`, FT 256) | 4,374,537 | 4.73 / 5.43 / 5.21 | 5.21M |
| nn2b (FT-512 build, `set evalfile`) | 4,518,329 | 3.91 / 4.34 / 4.36 | 4.34M |

nn2b runs at about 90% of nn1's nps (medians; gen3b datagen was paused for the bench). Single bench runs vary by up
to about 10% here: nn2a has the same architecture as nn1, so its speed is the same and the gap between them is noise.
The 747ab51 (FT 256) engine refuses nn2b: `cannot load ...nn2b.nnue: net ft 512, this build expects 256`.

## Matches vs nn1

Settings: `--movetime 0.1`, `-c 10`, 805 4-ply openings x 2 colours (`openings4.txt`, 1610 games), `set threads 1`,
`set hashsize 128`, `set book 0`, 2-7 piece endgame database (`egdb` symlink). The datagen (gen3b) was paused
(SIGSTOP) for each match, so the machine was otherwise idle.
- **nn2a vs nn1**: both sides use the same 747ab51 `regicide_cb.so`. A gets `set evalfile nn2a.nnue`; B uses the
  built-in nn1.
- **nn2b vs nn1**: A is the FT-512 scratch build of 747ab51 with `set evalfile nn2b.nnue`. B is a separate .so, the
  normal 256-wide 747ab51 build with its built-in nn1.

| A vs B | Elo [95% CI] (pentanomial) | LOS | W / D / L | pairs LL LD DD+WL WD WW |
|---|---|---|---|---|
| **nn2a vs nn1** | **+1.3** [-3.7, +6.3] | 69.27% | 302 / 1012 / 296 | 0 / 68 / 663 / 74 / 0 |
| **nn2b vs nn1** | **+8.0** [+2.7, +13.2] | 99.86% | 316 / 1015 / 279 | 0 / 57 / 656 / 90 / 2 |

Times: nn2b 03:13 to 03:28, nn2a 03:28 to 03:42. Average time per move was 0.050 s for every engine.

**Conclusion: nn2b (FT 512) is the only clear gain, +8.0 Elo over nn1 (LOS 99.9%).** It wins despite about 10% lower
nps, which agrees with its much lower validation loss. nn2a (same architecture, 100 epochs, plus gen3a data) is
+1.3 and not significant: its small loss gain over nn1 does not show in play. nn2c (lambda 0.5) was not matched.
To use nn2b as the built-in net, the engine's `FT` must change to 512 (`ft512.patch`) and `src/weights/net.bin` must
be replaced with `nn2b.nnue`.


## Files

- `nn2a.nnue`, `nn2c.nnue` (256 arch, load into 747ab51), `nn2b.nnue` (**FT 512: needs a build with `FT = 512`**;
  the 747ab51 engine rejects it). `config_<run>.json`: the trainer arguments and K.
- `ft512.patch`: the change for the FT-512 build.
- `logs/`: `nn2a.log`, `nn2b.log`, `nn2c.log`, `export_checks.txt`, `losses.txt`, `status.txt`.
- `matches/<run>-vs-nn1/`: `summary.txt`, `results.csv`.
- `scripts/`: `nn2_train.sh`, `nn2_post.sh`, `compare_loss.py`, `gen3b_chain.sh`.
