# NNUE round 4: n4a–n4d vs nn3e

**Result: n4c (FT 1024, 40 epochs) beats nn3e by +5.6 Elo [+0.9, +10.3] (95% CI, pentanomial), 1,610 games at
0.1 s/move.** n4d (FT 768, L1 64) is +3.2 [−1.3, +7.7]. n4c is the round-4 winner: its CI lower bound is above 0, so the
baseline (`t3/baseline-eb1037c-n4c`) uses master eb1037c built with `-DREGICIDE_NNUE_FT=1024` and n4c embedded.

## Setup

- **Code:** master `385611e`, which has the fast sparse FT→L1 NNUE code (AVX-VNNI).
  - Engines are built with the CMake width options: default (FT 512), `-DREGICIDE_NNUE_FT=768`,
    `-DREGICIDE_NNUE_FT=1024`, and `-DREGICIDE_NNUE_FT=768 -DREGICIDE_NNUE_L1=64`.
  - Trainer worktrees of 385611e differ only in the `INPUTS, FT, L1, L2, BUCKETS` line of `tools/nnue/nnue.py`
    (`scripts/trainer_*.patch`).
- **Verification of 385611e:**
  - Linux (clang): ctest 9/9 passes (7/7 for each width variant), and bench is 6,873,528.
  - Windows DLL (llvm-mingw clang 23.1.5): `regicide.exe` bench is 6,873,528, and nn2b is embedded.
  - `logs/build_385611e_and_variants.txt`.
- **Master eb1037c** (default FT 768, built-in nn3e) was also built and checked:
  - ctest 9/9;
  - bench 7,245,436 on both Linux and Windows `regicide.exe`;
  - the same count as the 385611e FT-768 build with `set evalfile nn3e.nnue`, so the nn3e side of the matches below
    plays exactly like eb1037c.
  - `logs/build_eb1037c.txt`.
- **Data** (`logs/data_args.txt`):
  - all of gen1, gen1b, gen1c, gen2a, gen2b, gen3a, gen3b, gen3c;
  - plus a whole-record prefix of gen3d: `gen3d_partial_18365696.bin`, 18,365,696 records snapshotted at 09:46 while
    gen3d was still running. gen3d itself finished at 10:34 with 28,599,457 positions, after training had started.
  - That gives 230,626,085 training positions plus 1,000,000 validation positions.
- **Training settings:** as nn2b/nn3:
  - `--epochs 40` (n4b: 80), `--epoch-size 100000000 --batch 16384 --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7
    --qat-epochs 2 --compile`, seed 1, K fitted (111.8);
  - CPU-capped with `--threads 1 --loader-threads 1`;
  - two GPU lanes on the RTX 5080 at about 3.2M pos/s each: lane 1 ran n4a → n4c → n4d, lane 2 ran n4b.
  - Wall times: n4a 20 min, n4b 41 min, n4c 22 min, n4d 16 min.

| Net | FT | L1 | Epochs | Trainer valid(int) at end |
|---|---|---|---|---|
| n4a | 768 | 32 | 40 | 0.515176 |
| n4b | 768 | 32 | 80 | 0.515061 |
| n4c | 1024 | 32 | 40 | 0.514355 |
| n4d | 768 | 64 | 40 | 0.514505 |

Export checks (`logs/export_checks.txt`): for every net, the Python integer model equals the engine's evaluation
exactly (max |diff| 0), both on the 200k gen2b slice and on 100k validation positions. Each `.nnue` file is byte-identical
to the final epoch's export.

## Loss on one shared validation set

The method is compare_loss (`scripts/compare_loss.py`):
- The validation set is n4a's: the 2% tail of each data file, 1M positions, seed 1. It is held out from every round-4
  net and also from nn3e and nn2b, which never trained on gen3c/gen3d and held out the same tails of the older files.
- Each net is scored with the engine's own integer eval (`regicide evaldata`) on the 385611e build of the matching width.
- The loss is BCE with λ 0.7 and K 111.83 (from n4a's config).

| Net | Arch | Loss | vs nn3e |
|---|---|---|---|
| nn2b | FT 512 | 0.519335 | +0.002936 |
| nn3e | FT 768 | 0.516399 | — |
| n4a | FT 768 | 0.515184 | −0.001215 |
| n4b | FT 768, 80 ep | 0.515075 | −0.001324 |
| **n4c** | **FT 1024** | **0.514373** | **−0.002026** |
| n4d | FT 768, L1 64 | 0.514509 | −0.001890 |

- More data alone, at the same architecture (n4a vs nn3e), is worth about 0.0012.
- Doubling the epochs (n4b) adds very little: 0.0001.
- The wider FT 1024 and the wider L1 64 add another 0.0007–0.0008.

## Speed (bench, 385611e builds)

The bench was run three times per build, with nothing else running: gen3d had finished, and the trainers and the other
agent's matches were done (`logs/bench.txt`).

| Build + net | Bench nodes | Mean nps | vs FT 512 | vs nn3e |
|---|---|---|---|---|
| FT 512, nn2b (built-in) | 6,873,528 | 4,591,332 | 100% | 116% |
| FT 768, nn3e | 7,245,436 | 3,966,057 | 86.4% | 100% |
| FT 768, n4a | 6,984,354 | 3,905,257 | 85.1% | 98.5% |
| FT 1024, n4c | 5,114,091 | 3,569,188 | 77.7% | 90.0% |
| FT 768 + L1 64, n4d | 5,544,944 | 3,299,398 | 71.9% | 83.2% |

The single-run spread is about ±5%. With the sparse FT→L1 code, FT 1024 costs only about 10% nps relative to FT 768.
The L1-64 build is the slowest per node.

## Matches vs nn3e

The two nets with the lowest losses, n4c and n4d, each played nn3e:
- **Builds:** each net on the 385611e build of its own width (`libregicide_cb.so` + `set evalfile`); nn3e on the
  385611e FT-768 build + `set evalfile nn3e.nnue`.
- **Settings:** `openings4.txt` (805 openings × 2 colours = 1,610 games), 0.1 s/move, `-c 10`, hash 128, threads 1,
  book off, 2–7 piece EGDB, `tools/match` from 385611e.
- **Machine load:** no datagen ran during the matches (gen3d had ended at 10:34, and gen4a started after them), and
  there was no training.

| Match | W / D / L | Pentanomial [LL LD DD/WL WD WW] | Elo (pentanomial) [95% CI] | LOS |
|---|---|---|---|---|
| **n4c vs nn3e** | 301 / 1034 / 275 | [0 50 679 76 0] | **+5.6 [+0.9, +10.3]** | 98.99% |
| n4d vs nn3e | 286 / 1053 / 271 | [0 49 692 64 0] | +3.2 [−1.3, +7.7] | 92.11% |

Run times: n4c 11:01–11:15, n4d 11:15–11:29. Full summaries and per-game rows are in `matches/`.

## Note on "datagen paused" in earlier rounds

While round 4 was setting up (09:39–09:49), the other agent's 0.3 s confirmation match was running, and its status log
said gen3d datagen had been paused with `kill -STOP` at 09:37. In fact the datagen was running:
- The process state was `S`, not `T`.
- Its log advanced from 169,000 to 185,000 games in that window.

The likely cause is that a non-job-control bash parent restarts a stopped foreground child with SIGCONT. So earlier
"datagen paused" matches may in fact have run with datagen active. This adds noise to those matches but does not bias
them, since both sides were affected equally.

Round 4 uses `scripts/pausekeeper.sh`, which stops both the datagen and its bash parent while the match lock is held.
In this round there was no datagen to pause during the matches.

## Files

- `n4a.nnue` … `n4d.nnue`, `config_n4*.json`
- `logs/`:
  - training logs;
  - `export_checks.txt`, `losses.txt`, `bench.txt`, `data_args.txt`;
  - build logs (385611e + variants, eb1037c);
  - `status.txt`, `jobs_status.txt`.
- `matches/n4c-vs-nn3e/`, `matches/n4d-vs-nn3e/`: `summary.txt`, `results.csv`
- `scripts/`: `r4_chain.sh` (train → checks → losses → bench → matches), `r4_build.sh`, `eb_build.sh`,
  `compare_loss.py`, `pausekeeper.sh`, `keepalive_r4.sh`, `launch.sh`, `build_win_variant.sh`, trainer width patches.
