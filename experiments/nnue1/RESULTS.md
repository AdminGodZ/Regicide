# nnue1: first NNUE probe (nn1) on the RTX 5080

Everything here was built and run from `master` @ `bd2ef1d`: the engine, `tools/nnue` (trainer, export, check),
datagen and the match tools. In bd2ef1d the built-in eval is the linear gen2 winner G2c, and there is no embedded book.
Machine: Core Ultra 9 285K + RTX 5080 (driver 621.14), WSL2 Ubuntu 24.04, clang 22 Release, PyTorch 2.11.0+cu128,
`torch.cuda.is_available()` = True.

## Setup checks

- `ctest`: 7/7 passed. `regicide bench`: 4,008,356 nodes. The count differs from a44591e (3,278,128) because the start
  position is no longer answered from a book.
- `check_export.py` with a random net, on a 200k-position slice of `gen2b_n10k.bin`: Python integer forward pass vs
  `regicide evaldata`: max |diff| 0. OK.
- `torch.compile` failed at first with `PermissionError: 'nvcc'`. A directory in the Windows part of the WSL `PATH`
  (`/mnt/c/...`) made the `nvcc` lookup fail with EACCES. Dropping the `/mnt/c` entries from `PATH` fixed it, and the
  training scripts do this.

## Training

```
python tools/nnue/train.py --data gen1_n10k.bin --data gen1b_n10k.bin --data gen1c_n10k.bin --data gen2a_n10k.bin \
    --data gen2b_n10k.bin --out runs/nn1 --epochs 40 --epoch-size 100000000 --batch 16384 \
    --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 --qat-epochs 2 --compile
```

- Data: the 5 files hold 128.85M positions; 126,277,323 are used for training (the last 2% of each file is held out).
  Validation uses 1,000,000 positions sampled from those tails (seed 1). The fitted K is 116.6.
- Runtime: 01:02:45 to 01:16:42 (14 min), about 4.8M pos/s, 4B samples. The README's ETA concern did not apply, so
  the full 40 epochs ran. gen3a datagen (12 threads) ran on the CPU at the same time.
- Log: `logs/nn1.log`. Config: `config.json`.

| Epoch | lr | train | valid (float) | valid (int) |
|---|---|---|---|---|
| 1  | 9.99e-04 | 0.543113 | 0.537975 | 0.538222 |
| 5  | 9.63e-04 | 0.536272 | 0.535592 | 0.535802 |
| 10 | 8.56e-04 | 0.535531 | 0.534747 | 0.535138 |
| 20 | 5.06e-04 | 0.534917 | 0.534191 | 0.534486 |
| 30 | 1.55e-04 | 0.534589 | 0.533923 | 0.534686 |
| 38 | 1.61e-05 | 0.534479 | 0.533819 | 0.534242 |
| 39 (QAT) | 1.15e-05 | 0.534669 | 0.534173 | 0.534022 |
| **40 (QAT, exported)** | 1.00e-05 | 0.534723 | 0.534176 | **0.534373** |

The float-to-integer gap is 0.0002-0.0008 before QAT. Epoch 39's integer net (0.534022) was a little better than the
final one (0.534373). The exported net is epoch 40.

### Validation loss: nn1 vs the linear eval on the same positions

The two trainers split the data differently. The linear trainer (`tools/train.cpp`) shuffles all records and holds out
a random 2% (gen2: 4 files, K 108.7). The NNUE trainer holds out the last 2% of each file and uses K fitted on its own
data (116.6). Their logged losses are therefore **not comparable** (G2c: 0.535884 on the linear split; nn1: 0.534373
on the NNUE split).

For a like-for-like number, I rebuilt the NNUE trainer's exact validation set (seed 1, the same 1M positions; the nn1
loss below reproduces the log's final `valid(int)` to 1e-6). I scored every eval with `regicide evaldata`, which
gives the engine's integer static eval, using the trainer's loss with K 116.6 and lambda 0.7 (`compare_loss.py`):

| Eval | Valid loss (NNUE validation set, K 116.6) |
|---|---|
| **nn1** | **0.534374** |
| G2d (linear, gen2) | 0.541409 |
| G2c (linear, bd2ef1d built-in) | 0.541466 |
| w_lr0.03_wd0 (linear, a44591e built-in) | 0.544992 |

nn1 is 0.0071 below the built-in linear eval. That is about 3x the gap between the a44591e and bd2ef1d built-ins
(0.0035), and that gap was worth +14 Elo. Two caveats about the linear numbers:
1. The linear evals were trained on a random 98% of gen1..gen2a, so most of the tail positions from those 4 files were
   in their training set. That favours the linear evals.
2. They were not trained on gen2b, and they were fitted with K 108.7, not 116.6. That counts against them.

## Export check (must be exact)

`check_export.py --checkpoint runs/nn1/last.pt --net nn1.nnue`. The exported `nn1.nnue` is byte-identical to the
trainer's `net_e040.nnue` (76,108 bytes, sha256 `3b227143f42bf2f6...`).
- 1000 positions of `gen2b_n10k.bin`: Python int vs engine max |diff| 0, 0 positions differ. **OK.**
- 100,000 positions of the validation set: max |diff| 0. **OK.**

## Bench (single thread; gen3a datagen paused)

| | bench (default) nodes | nps, 3 runs | bench 15 nodes | nps |
|---|---|---|---|---|
| built-in (G2c, linear) | 4,008,356 | 5.51M / 5.49M / 5.71M | 2,496,518 | 6.07M |
| `set evalfile nn1.nnue` | 4,082,499 | 4.92M / 4.85M / 5.08M | 2,657,193 | 5.05M |

The NNUE is about 11% slower on the default bench (medians 4.92M vs 5.51M) and about 17% slower at bench 15.

## Match: nn1 vs bd2ef1d built-in

Both sides run the **same** bd2ef1d `regicide_cb` `.so`; the NNUE side also gets `set evalfile <abs path>/nn1.nnue`.
In a cbhost check, the engine answered `reply 1 nnue` to that command, and it answers `reply 0 cannot open ...` for a
bad path. The settings are those of the gen2 matches:
- `--movetime 0.1`, `-c 10`, 805 4-ply openings x 2 colours (`openings4.txt`, 1610 games).
- `set threads 1`, `set hashsize 128`, `set book 0` for both engines.
- 2-7 piece endgame database (via an `egdb` symlink).

gen3a datagen was paused (SIGSTOP) for the whole match, so the machine was otherwise idle.

MATCH_TABLE

The control match first planned (1a32912 built-in vs a44591e built-in, for the circular-capture fix) was dropped when
the plan changed to the same-binary comparison. It was stopped after 1095 of 1610 games at +0.6 [−4.0, +5.3]
(+217 =662 −216), so the fix shows no measurable Elo effect. That result is not used.

## gen3a datagen

`datagen --games 300000 --nodes 10000 --threads 12 --seed 41 --out gen3a_n10k.bin` (bd2ef1d, built-in eval = G2c,
no `--evalfile`, no egdb). It started at 01:02:45 alongside the GPU training and was paused for the nn1 match
(01:21:41 to 01:37:16). At 01:42 it was still running, with the last progress line `games 84000  positions 8842317  black +18889 =44535 -20576`.
Output: `~/regicide-data/train/gen3a_n10k.bin` (not committed).

## Files

- `nn1.nnue`: the exported net (epoch 40). `config.json`: the trainer arguments and the fitted K.
- `logs/nn1.log`: the training log. `logs/status.txt`: the job status file.
- `matches/nn1-vs-bd2ef1d/`: `summary.txt`, `results.csv`.
- `scripts/`: `nn1_train.sh`, `nn1_post.sh` (export check, loss comparison, bench), `compare_loss.py`,
  `nnue_match.sh`, `nn1_match.sh`, `run_gen3a.sh`.
