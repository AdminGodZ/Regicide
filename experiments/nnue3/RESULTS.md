# nnue3: NNUE round 3 (nn3a, nn3b, nn3c, nn3d) vs nn2b

All runs used `master` @ `825f993`. Its built-in net is nn2b (FT 512, `experiments/nnue2/nn2b.nnue`), with no
opening book. Two candidates need another architecture. They were trained and played with scratch worktrees of
825f993 that change one constant in `src/nnue.hpp` and the matching tuple in `tools/nnue/nnue.py`:
- `ft1024.patch`: `FT = 1024` (nn3c);
- `l1_64.patch`: `L1 = 64` (nn3d). L1 is a free constant on both sides: the engine's `affine<N, M>` kernels only
  need `L1 % 32 == 0`, and the Python model, quantiser and RGNN reader/writer are written in terms of `L1`.

In both worktrees `src/weights/net.bin` is empty, because the embedded nn2b does not fit their architecture. Each
then uses the linear fallback unless given `set evalfile`.

Checks on the scratch builds (`logs/build_825f993_and_variants.txt`):
- Both pass ctest (7/7).
- Their bench with the linear fallback is 4,008,356 nodes.
- `check_export.py` with a random net is exact (max |diff| 0).
- Each rejects nn2b (`net ft 512, this build expects 1024`; `net l1 32, this build expects 64`).

The 825f993 build itself (built-in nn2b) passes ctest with bench **4,518,329**; its llvm-mingw Windows build gives the
same bench. Hardware and software: RTX 5080, PyTorch 2.11.0+cu128, WSL2, clang 22.

## Data

All finished files: gen1, gen1b, gen1c, gen2a, gen2b, gen3a (complete this time, 30.0M positions) and **gen3b**
(747ab51 with built-in nn1, seed 43, 280,968 games, 29,399,508 positions, finished 04:49).
- Train: 184,521,230 positions.
- Validation: 1,000,000 positions sampled (seed 1) from the last 2% of each of the 7 files.
- Fitted K: 116.6 (the same for all 4 runs, which share the data and seed).

nn2b never trained on any validation position, so it is a fair reference on this set:
- For gen1 to gen2b, it held out the same last 2% of each file.
- From gen3a it used only the first 11.8M records (`gen3a_partial_11831552.bin`), and the gen3a tail starts at
  29.4M.
- It never saw gen3b.

## Training

All runs use the nn2b settings: `--epoch-size 100000000 --batch 16384 --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7
--qat-epochs 2 --compile`. CPU threads were not capped (torch default, 2 loader threads). The runs went one after
another on the GPU, alongside the gen3c datagen (12 threads), at 3.9 to 4.4M pos/s (about 25 s per epoch).

| Run | Change vs nn2b | Epochs | Time |
|---|---|---|---|
| nn3a | none (more data) | 40 | 04:50 to 05:06 |
| nn3c | FT 1024 | 40 | 05:06 to 05:22 |
| nn3d | L1 64 | 40 | 05:22 to 05:37 |
| nn3b | none (more data) | 80 | 05:37 to 06:08 |

Valid loss (int) from the training logs, all measured on the same validation set:

| Epoch | nn3a | nn3b (80 ep) | nn3c (FT 1024) | nn3d (L1 64) |
|---|---|---|---|---|
| 1  | 0.533991 | 0.532451 | 0.531621 | 0.533011 |
| 10 | 0.529260 | 0.529660 | 0.526757 | 0.528216 |
| 20 | 0.528694 | 0.528215 | 0.526549 | 0.527424 |
| 30 | 0.527869 | 0.528294 | 0.525913 | 0.527386 |
| 40 | **0.527860** | 0.527874 | **0.525323** | **0.526834** |
| 60 | | 0.528028 | | |
| 80 | | **0.527487** | | |

### Comparison on one validation set

Every net was scored on the same 1M validation positions with lambda 0.7 and K 116.65 (nn3a's config;
`scripts/compare_loss.py`). Scores come from `regicide evaldata`, the engine's integer eval, using the build of the
net's own width. nn2b is the 825f993 built-in, which is byte-identical to `experiments/nnue2/nn2b.nnue`.

| Net | Arch | Valid loss | vs nn2b |
|---|---|---|---|
| **nn3c** | FT 1024 | **0.525319** | −0.00347 |
| **nn3d** | FT 512, L1 64 | **0.526833** | −0.00196 |
| nn3b | FT 512, 80 ep | 0.527465 | −0.00132 |
| nn3a | FT 512, 40 ep | 0.527885 | −0.00090 |
| nn2b (reference) | FT 512 | 0.528788 | |

- Width again gives the largest loss gain. nn3c's gain over nn2b (0.0035) is larger than nn2b's over nn1 (0.0027 on
  the round-2 set).
- nn3d's wider first hidden layer also helps, about 2x the gain of more data alone (nn3a).
- More data and twice the epochs (nn3b) give the smallest gains.

## Export checks (must be exact)

For each run, `check_export.py --checkpoint last.pt --net <run>.nnue` was run with the build of the matching
architecture. It was checked on 1000 positions of `gen2b_n10k.bin` and on 100,000 validation positions:
- Python int vs engine: max |diff| 0 in every case. **OK**.
- Each exported net is byte-identical to the trainer's final `net_eNNN.nnue`.

See `logs/export_checks.txt`.

## Bench and nps (single thread)

`bench.txt` (two sets of 3 runs, 06:09 and 06:11): gen3c datagen paused (SIGSTOP), no training, machine otherwise idle.

| Net | Build | bench nodes | nps, 6 runs (M) | median | vs nn2b |
|---|---|---|---|---|---|
| nn2b (built-in) | 825f993 | 4,518,329 | 4.16 3.91 3.99 3.90 4.12 4.16 | 4.06M | 100% |
| nn3a | 825f993 + `set evalfile` | 4,102,497 | 4.01 3.98 4.02 4.01 3.79 4.37 | 4.01M | 99% |
| nn3b | 825f993 + `set evalfile` | 3,774,421 | 3.84 3.85 4.03 3.91 3.90 4.50 | 3.90M | 96% |
| nn3c | FT 1024 build | 4,069,124 | 2.90 2.85 2.96 2.98 2.87 3.11 | **2.93M** | **72%** |
| nn3d | L1 64 build | 4,407,908 | 3.23 3.25 3.52 3.45 3.24 3.54 | **3.35M** | **83%** |

Measured cost of each width: FT 1024 runs at 72% of FT 512's nps, and L1 64 at 83%. nn3a and nn3b have nn2b's
architecture, so their speed equals nn2b's within noise.

## Selection

The rule was to match the best two by loss (nn3c, nn3d), plus nn3c if its loss gain beats its nps cost. nn3c is
already in the best two, so two matches were played. nn3a and nn3b were not matched: they have the smallest gains, and
nn2b went from +8 Elo over nn1 with 0.0027 less loss.

## Matches vs nn2b

Settings:
- `--movetime 0.1`, `-c 10`, 805 4-ply openings x 2 colours (`openings4.txt`, 1610 games).
- `set threads 1`, `set hashsize 128`, `set book 0`, 2-7 piece endgame database (`egdb` symlink).
- The gen3c datagen was paused (SIGSTOP) for both matches, so the machine was otherwise idle.
- A is the scratch build of 825f993 with the candidate's width, plus `set evalfile <net>`.
- B is a separate copy of the normal 825f993 `regicide_cb.so` with its built-in nn2b.

| A vs B | Elo [95% CI] (pentanomial) | LOS | W / D / L | pairs LL LD DD+WL WD WW |
|---|---|---|---|---|
| **nn3c (FT 1024) vs nn2b** | **+0.9** [−4.1, +5.9] | 63.24% | 300 / 1014 / 296 | 0 / 68 / 665 / 72 / 0 |
| **nn3d (L1 64) vs nn2b** | **−0.0** [−5.2, +5.2] | 50.00% | 299 / 1012 / 299 | 0 / 76 / 653 / 76 / 0 |

Times: nn3c 06:11 to 06:25, nn3d 06:25 to 06:40. Average time per move was 0.050-0.051 s for every engine.

**Conclusion: neither candidate beats nn2b. Both are within ±1 Elo, with CIs of about ±5. nn2b stays the built-in
net.**
- In both, the loss gain is cancelled by the nps cost:
  - nn3c's loss gain is larger than the one that made nn2b +8 over nn1, but it runs at 72% of the speed.
  - nn3d's gain is about 60% of that, at 83% of the speed.
- At 0.1 s/move the bigger nets just break even. They could pay off at longer time controls, or if the FT update and
  the L1 kernel get faster at these widths. That would need its own match.
- More data and longer training at FT 512 (nn3a, nn3b) give only 0.0009-0.0013 less loss. By the round-2 rate that is
  worth about 3-4 Elo at most, so they were not matched.

## Files

- `nn3a.nnue`, `nn3b.nnue`: FT 512, the same architecture as 825f993, so they load into master with `set evalfile`.
- `nn3c.nnue`: **FT 1024, needs `ft1024.patch`.**
- `nn3d.nnue`: **L1 64, needs `l1_64.patch`.**
- `config_<run>.json`: the trainer arguments and K.
- `matches/<A>-vs-nn2b/`: `summary.txt` and `results.csv` per match.
- `logs/`:
  - `<run>.log`: training logs.
  - `export_checks.txt`, `losses.txt`, `bench.txt`.
  - `build_825f993_and_variants.txt`: 825f993 and the scratch builds (ctest, bench, random-net export checks).
  - `status.txt`: pipeline timestamps.
- `scripts/`:
  - `r3_chain.sh`: the whole pipeline (wait for gen3b, start gen3c, train, check, score, bench, match).
  - `r3_build.sh`: the builds.
  - `gen3c.sh`, `compare_loss.py`, `keepalive_r3.sh`.

## Addendum: nn3e (FT 768)

FT 768 sits between nn2b and nn3c, which may give a better trade between loss and nps. Like nn3c, it was trained and
played with a scratch worktree of 825f993: `ft768.patch` sets `FT = 768`, and `src/weights/net.bin` is empty.
- **Build checks** (`logs/build_ft768.txt`): ctest 7/7, bench with the linear fallback 4,008,356 nodes, random-net
  `check_export` exact.
- **Training:** the same data, settings and validation set as nn3a, 40 epochs. It ran 06:47 to 07:03, during the
  ae210fb baseline, so it was capped at `--threads 1 --loader-threads 1` (OMP_NUM_THREADS=1).
- **Export:** exact (max |diff| 0) on 1000 gen2b positions and on 100,000 validation positions. `nn3e.nnue` is
  identical to `net_e040.nnue` (`logs/export_checks_nn3e.txt`).

| Epoch | 1 | 10 | 20 | 30 | 40 |
|---|---|---|---|---|---|
| nn3e valid(int) | 0.532483 | 0.527586 | 0.526948 | 0.526670 | **0.526246** |

The loss on the shared validation set is **0.526238** (`logs/losses_nn3e.txt`). That is 0.00255 below nn2b, between
nn3d (0.526833) and nn3c (0.525319).

Bench (`logs/bench_nn3e.txt`, 3 rounds at 08:21, datagen paused, otherwise idle):

| Net | Build | bench nodes | nps (M) | median | vs nn2b |
|---|---|---|---|---|---|
| nn2b | 825f993 | 4,518,329 | 4.46 3.86 3.94 | 3.94M | 100% |
| nn3e | FT 768 build | 4,586,075 | 3.40 3.33 3.34 | 3.34M | **85%** |
| nn3c | FT 1024 build | 4,069,124 | 2.83 2.91 2.89 | 2.89M | 73% |

The match used the same settings as the nn3c and nn3d matches: 1610 games at 0.1 s/move, with datagen paused.

| A vs B | Elo [95% CI] (pentanomial) | LOS | W / D / L | pairs LL LD DD+WL WD WW |
|---|---|---|---|---|
| **nn3e (FT 768) vs nn2b** | **+0.2** [−4.9, +5.4] | 53.26% | 300 / 1011 / 299 | 0 / 74 / 656 / 75 / 0 |

Time: 08:22 to 08:36. **nn3e is level with nn2b as well.** On the 825f993 code, every wider net (FT 768, FT 1024,
L1 64) loses its lower loss to its nps cost at 0.1 s/move.

Files: `nn3e.nnue` (**FT 768, needs `ft768.patch`**), `config_nn3e.json`, `logs/nn3e.log`,
`matches/nn3e-vs-nn2b/`, `scripts/extra.sh` (the nn3e and gen3d job).
