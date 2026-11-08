# nnue_v2: second NNUE architecture (both views into L1, bucketed hidden layers)

Branch `t3/nnue-v2`, from master `3c87728` (built-in net n4c, bench 7,338,658). Mac work only: code, ctest, short
benches, smoke trainings. No net was trained for strength yet; the commands for the RTX 5080 are at the end.

**Summary**
- The engine now reads the net's architecture from the RGNN header. One build loads v1 nets of any supported width
  (no more `-DREGICIDE_NNUE_FT/L1` builds) and the new v2 nets. v1 nets load and evaluate exactly as before.
- v2 = the two standard ideas: L1 reads the side to move's FT output **and** the opponent's (shared FT weights), and
  L1, L2 and the output layer each have one set of weights per piece-count bucket.
- The trainer takes `--arch v2` plus `--ft/--l1/--l2/--buckets`. Exports match the engine exactly (max |diff| 0).
- Speed on the M5: **v2 2×512 runs at the speed of v1 FT 1024** (same L1 input width, half-width accumulator
  updates). v2 2×1024 runs at 60% (78% with L1 16).

## Commits

| Commit | Change |
|---|---|
| `ce8f60c` | Engine: runtime architecture from the header, v2 forward pass, accumulator stack updates both views for v2, header checks with reasons; ctests per architecture |
| `a2279df` | Trainer: `--arch v2`, sizes, v2 model / quantisation / RGNN / integer reference; export and check_export take the arch from the checkpoint |
| `7aac617` | Engine speed: accumulator loops compiled for FT 512/768/1024 (v1 back to master's nps) |
| (this) | README, this file, `scripts/train_v2.sh` |

## Design

### Network

```
inputs   128 = [our man, our king, their man, their king] x 32 squares, seen from one colour
FT       128 -> FT per point of view, int16 (x255), SCReLU clamp(acc, 0, 255)^2 >> 9 -> 0..127
L1       N -> L1, int8 (x64), int32 sums, clamp(sum >> 6, 0, 127)
L2       L1 -> L2, same
output   L2 -> 1, int16 (x64); score = out * out_scale / (127 * 64), clamped to +-14999

v1: N = FT (side to move's view). Output layer bucketed by piece count, hidden layers not.
v2: N = 2 FT: [side to move's view | opponent's view], same FT weights for both.
    L1, L2 and output each have `buckets` weight sets; bucket = min((pieces - 1) * buckets / 24, buckets - 1).
```

The quantisation is unchanged, so every kernel and every exactness argument carries over. With 2 FT inputs the
largest L1 sum is 2048 · 127 · 127 = 33M, well inside int32.

### RGNN file

The header is the same 44 bytes as before, with the same fields; the version tells the layouts apart:

```
"RGNN"  u32 version (1 | 2), inputs (128), ft (per view), l1, l2, buckets, activation (1), qa (255), qb (64), out_scale
ft_bias    i16[FT]
ft_weight  i16[128][FT]
l1_bias    i32[HB][L1]          HB = 1 in v1, buckets in v2
l1_weight  i8 [HB][L1][N]       N = FT in v1, 2 FT in v2 (stm columns first)
l2_bias    i32[HB][L2]
l2_weight  i8 [HB][L2][L1]
out_bias   i32[buckets]
out_weight i16[buckets][L2]
```

A v1 file is byte for byte what earlier versions wrote: n4c round-trips through `read_rgnn`/`write_rgnn` unchanged.

### Engine

- `Network::load` parses the header into an `Arch` and rejects, with the reason:
  - versions other than 1 and 2;
  - inputs, activation, qa or qb that differ from the build's;
  - FT that is not a multiple of 128, or wider than `REGICIDE_NNUE_MAX_FT` (default 1024 per view);
  - L1 or L2 other than 16, 32 or 64;
  - more than 24 buckets;
  - a size that does not fit the header.
- Examples: `net v2 2x512 -> 48 -> 32 -> 1, 8 buckets (all layers): l1 and l2 must be 16, 32 or 64`, and a v1
  header on v2 data gives `net v1 1024 -> 32 -> 32 -> 1, 8 output buckets: size 799308, expected 298828`.
  Master builds reject v2 nets with `net version 2, this build expects 1`.
- Weights live in 64-byte-aligned heap arrays sized from the header. The accumulator stack still holds
  `int16[2][MAX_FT]` per ply, so its size is unchanged (1024 per view = what n4c already used).
- The forward pass is a template over (L1, L2), chosen by a switch on the header's widths. The accumulator loops
  (refresh, update, SCReLU transform) are compiled for FT 512, 768 and 1024, and run with a runtime bound otherwise.
- **v2 forward:** SCReLU of `acc.v[stm]` into `a[0, FT)` and of `acc.v[~stm]` into `a[FT, 2 FT)`. The NEON/AVX2
  non-zero chunk list continues across both halves, so the sparse FT→L1 kernel (`affine_chunks`) runs unchanged over
  the 2 FT inputs. Bucket `b` selects the L1 and L2 weight and bias blocks and the output weights.
- **FT reordering:** the load-time reordering for sparsity is shared by the two views. Their weights are shared, so
  for v2 it is fitted on samples from both views, and L1's columns are permuted the same way in each half.
- **Accumulator stack:** `get()` brings the side to move's view up to date, and for v2 also the opponent's. v1 keeps
  the lazy single-view behaviour.
- **Build flags for a matching engine:** none. Any v1/v2 net with FT ≤ 1024 per view, L1/L2 in {16, 32, 64} and
  ≤ 24 buckets loads into the default build with `set evalfile` (CheckerBoard: `enginecommand("set evalfile …")`).
  For FT > 1024 per view, configure with `-DREGICIDE_NNUE_MAX_FT=2048`, which doubles the accumulator stack. To
  embed a v2 net, copy it to `src/weights/net.bin` and rebuild; no constants change.

### Trainer (`tools/nnue`)

- `nnue.Arch(version, ft, l1, l2, buckets)`, with defaults v1 = n4c's (1024, 32, 32, 8) and v2 = (512, 32, 32, 8).
- `Net(arch)`:
  - One `ft` Linear, applied to both views' inputs and concatenated.
  - `l1`/`l2` are Linears with `HB·L1` / `HB·L2` outputs: all buckets are computed, and `select()` keeps each row's
    bucket.
  - Bucketed layers start with all buckets equal.
  - With HB = 1, the state dict has exactly the v1 names and shapes, so old checkpoints load (`arch_of_state` infers
    the arch from the weight shapes).
- `quantise`, `write_rgnn` and `read_rgnn` work for both versions. `forward_int` is the integer reference (numpy
  int64) and mirrors `Network::evaluate`, including the bucket selection.
- `train.py`:
  - New options `--arch v1|v2 --ft --l1 --l2 --buckets`.
  - `--resume` refuses a checkpoint of another arch.
  - `config.json` records the arch.
  - Data format, λ/K target, cosine LR, QAT epochs and all other options are unchanged.
- `export.py` and `check_export.py --checkpoint` take the arch from the checkpoint. Without a checkpoint,
  `check_export.py` builds a random net of `--arch/--ft/--l1/--l2/--buckets`.

## Checks

**ctest** (12/12 pass) on three builds:
- native NEON sdot (`build`);
- portable arm64, `-march=armv8-a`, no dotprod (`build-portable`, nnue tests);
- x86_64 `-march=haswell`, i.e. the AVX2 kernel, under Rosetta (`build-x86`, nnue tests).

The nnue ctests:

| ctest | Net (random) |
|---|---|
| `nnue` | v1 1024 → 32 → 32, 8 buckets; also `Evaluator::from_file` and the embedded net |
| `nnue_ft512` | v1 512 → 32 → 32 |
| `nnue_l1_64` | v1 768 → 64 → 32 |
| `nnue_v2` | v2 2×512 → 32 → 32, 8 buckets |
| `nnue_v2_wide` | v2 2×1024 → 16 → 32, 8 buckets |
| `nnue_v2_shapes` | v2 2×256 → 64 → 16, 4 buckets |

Each one checks:
- the engine's forward pass against a naive 64-bit reference written from the file layout;
- lazy incremental accumulators against a full refresh, over random tree walks (34,971 positions);
- colour-flip symmetry (300 positions);
- 17 malformed headers, each rejected with a reason: version 3, the other version with the same sizes, inputs, FT
  not a multiple of 128, FT over the maximum, FT 0, L1 48, L2 8, buckets + 1, 0 buckets, activation, QA, QB, truncated,
  trailing byte, bad magic, header only. The reason must name the bad field.

**v1 unchanged:**
- The built-in n4c benches **7,338,658** on all three builds.
- `evaldata` on all 29,256 smoke positions is identical to master's.
- nps paired with master (5 alternating runs each, `logs/v1_vs_master.txt`): −0.3% (4.368M vs 4.379M mean), inside
  the run-to-run noise. Before 7aac617 it was −1%.

**check_export (Python integer net vs `regicide evaldata`): max |diff| 0 in every case.**
- Random nets on 3,000 smoke positions:
  - v1 1024/32, v1 512/64;
  - v2 2×512/32/32 with 8 buckets, v2 2×1024/16, v2 2×256/64/16 with 4 buckets, v2 2×512 with 1 bucket.
- Smoke-trained nets on all 29,256 positions: v1 1024, v2 2×512, v2 2×1024.
- n4c (`read_rgnn` + `forward_int`) on all 29,256 positions.

## Smoke run (M5 Pro CPU)

```sh
build/datagen --games 300 --threads 2 --out smoke.bin            # 16 s, 29,256 positions
python tools/nnue/train.py --data smoke.bin --out v2run --arch v2 --epochs 6 --epoch-size 196608 --batch 4096 \
    --warmup 20 --qat-epochs 2 --threads 4 --loader-threads 1 --valid-frac 0.05      # 5 s
python tools/nnue/export.py v2run/last.pt v2smoke.nnue           # identical to v2run/net_e006.nnue
python tools/nnue/check_export.py --engine build/regicide --data smoke.bin --checkpoint v2run/last.pt --n 29256
printf 'set evalfile v2smoke.nnue\nbench\nquit\n' | build/regicide   # eval: nnue, bench 9,982,882 nodes
```

- The same with `--ft 1024` ran 12 epochs in 14 s.
- The losses mean nothing on 29k positions: the nets overfit after epoch 2.
- CPU training speed (4 threads, batch 4096):
  - v1 1024: about 530k pos/s;
  - v2 2×512: 330k pos/s (0.62×);
  - v2 2×1024/L1 16: 215k pos/s (0.4×).
- The v2 trainer computes all 8 buckets of L1 and L2 and then selects, so L1 costs 8× v1's. On the GPU, where the
  earlier runs were limited by sampling, the slowdown should be smaller.

## Speed (M5 Pro, `bench`, 3 alternating rounds, single thread)

Random nets use one generator (`check_export.py --net`, seed 3) for every arch, so v1 and v2 are compared at the same
activation density. Random nets are much denser than trained ones (89% vs n4c's 64% non-zero FT chunks), which makes
FT→L1 slower for all of them alike. Smoke-trained nets (seconds of training) are shown as a second, equally rough,
sample. Raw data: `logs/bench_rounds.txt`, `logs/nnuebench.txt`.

| Net | Bench nodes | nps (median) | vs v1 1024 of the same kind |
|---|---|---|---|
| n4c, built-in (trained v1 1024) | 7,338,658 | 4.23M (4.38M earlier, quieter machine) | |
| random v1 1024 → 32 → 32 | 16,803,906 | 3.40M | 100% |
| **random v2 2×512 → 32 → 32, 8 buckets** | 12,698,368 | **3.42M** | **100%** |
| random v2 2×512, 1 bucket | 7,205,631 | 3.61M | 106% |
| **random v2 2×1024 → 32 → 32, 8 buckets** | 31,433,099 | **2.05M** | **60%** |
| random v2 2×1024 → 16 → 32, 8 buckets | 6,959,389 | 2.64M | 78% |
| smoke v1 1024 | 13,125,531 | 3.39M | 100% |
| smoke v2 2×512, 8 buckets | 9,982,882 | 3.47M | 102% |
| smoke v2 2×1024, 8 buckets | 11,189,776 | 2.03M | 60% |

Per stage (`regicide nnuebench <net> smoke.bin 1024`, ns per call):

| Net | evaluate | transform | FT→L1 | L1→L2 | update, 1 move, both views | refresh, both views | non-zero FT chunks |
|---|---|---|---|---|---|---|---|
| n4c | 193 | 49 | 148 | 7.0 | 115 | 483 | 163 / 256 |
| random v1 1024 | 246 | 49 | 202 | 7.0 | 115 | 481 | 229 / 256 |
| random v2 2×512 | 246 | 44 | 210 | 7.1 | 54 | 238 | 228 / 256 |
| random v2 2×512, 1 bucket | 244 | 44 | 202 | 7.0 | 54 | 238 | 228 / 256 |
| random v2 2×1024 | 473 | 84 | 406 | 7.1 | 115 | 482 | 453 / 512 |
| random v2 2×1024, L1 16 | 319 | 84 | 255 | 4.4 | 115 | 483 | 457 / 512 |

What is slower, and why:
- **FT→L1** costs about 0.9 ns per non-zero 4-byte chunk on every arch, since it is bound by loading weights (128 B
  per chunk at L1 32).
  - 2×512 has as many inputs as v1 1024, so it costs the same, if the trained v2 nets turn out as sparse as n4c. The
    opponent's half is a new unknown.
  - 2×1024 doubles it, and this is most of its 40% loss.
  - L1 16 halves the bytes per chunk: 255 ns instead of 406.
- **Transform** (SCReLU plus the chunk list) scales with the total width: the same at 2×512, double at 2×1024.
- **Accumulator updates:**
  - v2 needs both views at every evaluation, where v1 lazily computes only the side to move's at a leaf.
  - At 2×512 each view is half as wide, so v2 still does no more update work than v1 1024.
  - At 2×1024 every view is full width, and the bench shows it: the evaluation stages alone do not account for the
    full 40%.
- **Bucketed hidden layers** cost about 5% in the search (3.42M vs 3.61M), but nothing in the microbenchmark (its
  positions come game by game, mostly in one bucket). This is likely cache: 8× the L1 weights (256 KB at 2×512,
  512 KB at 2×1024, instead of 32 KB).
- Not yet optimised for v2:
  - one pass that updates both views (the weight rows differ per view, so the gain is limited to the stack walk);
  - an AVX2 transform with the chunk search fused in (the NEON one has it).

Expectation for the 285K: the same ratios should hold roughly, because FT→L1 and the updates are the bulk on both
machines. VNNI made FT→L1 cheaper there, so the 2×1024 penalty may be a little smaller. **The v2 code path has not
run on x86 hardware yet**, only AVX2 under Rosetta: run ctest on the 285K first, since the v2 ctests cover the
AVX-VNNI kernel against the reference.

## Training v2 on the RTX 5080 (WSL)

Setup as in `tools/nnue/README.md` (the venv at `~/regicide-nnue/.venv` from the earlier rounds works). In WSL:

```bash
cd ~/regicide && git fetch origin && git checkout t3/nnue-v2      # or a fresh worktree of it
cmake -S . -B build -G Ninja && cmake --build build && ctest --test-dir build    # 12/12, incl. nnue_v2*
build/regicide bench | tail -1                                     # must be 7338658 nodes (built-in n4c)

# Random-net export checks, v1 and v2 (each about 2 s):
for a in "--arch v1" "--arch v2" "--arch v2 --ft 1024 --l1 16"; do
  ~/regicide-nnue/.venv/bin/python tools/nnue/check_export.py --engine build/regicide \
      --data ~/regicide-data/train/gen2b_n10k.bin $a
done
```

The script trains, runs the exact export check on 100,000 positions and benches the net. It uses every
`~/regicide-data/train/*.bin` except `*partial*` snapshots (they duplicate a prefix of a finished file); look at
`runs/<name>.data_args.txt`. Settings are the round-4 ones: 40 epochs of 100M, batch 16384, lr 1e-3 → 1e-5 cosine,
wd 0.01, λ 0.7, K fitted, 2 QAT epochs, `--compile`, `--threads 1 --loader-threads 1`.

```bash
# v2a: the main candidate. Same speed as n4c, so any loss gain should turn into Elo.
experiments/nnue_v2/scripts/train_v2.sh v2a --arch v2 --ft 512 --l1 32 --l2 32 --buckets 8

# v2b: twice the FT per view, L1 16 to limit the cost (78% of n4c's nps on the M5).
experiments/nnue_v2/scripts/train_v2.sh v2b --arch v2 --ft 1024 --l1 16 --l2 32 --buckets 8

# Optional ablations: which half of v2 helps.
experiments/nnue_v2/scripts/train_v2.sh v2c --arch v2 --ft 512 --buckets 1     # both views, no hidden buckets
experiments/nnue_v2/scripts/train_v2.sh v2d --arch v2 --ft 1024 --l1 32        # 2x1024 / 32: 60% nps
```

Expanded, v2a is:

```bash
PY=~/regicide-nnue/.venv/bin/python
export PATH=$(echo "$PATH" | tr ":" "\n" | grep -v "^/mnt/c" | paste -sd: -) OMP_NUM_THREADS=1
DATA=$(for f in ~/regicide-data/train/*.bin; do case $f in *partial*) ;; *) echo --data $f;; esac; done)
$PY tools/nnue/train.py $DATA --out ~/regicide-nnue-v2/runs/v2a --arch v2 --ft 512 --l1 32 --l2 32 --buckets 8 \
    --epochs 40 --epoch-size 100000000 --batch 16384 --lr 1e-3 --lr-final 1e-5 --wd 0.01 --lambda 0.7 \
    --qat-epochs 2 --compile --threads 1 --loader-threads 1 2>&1 | tee ~/regicide-nnue-v2/runs/v2a.log
$PY tools/nnue/check_export.py --engine build/regicide --data ~/regicide-data/train/gen2b_n10k.bin \
    --checkpoint ~/regicide-nnue-v2/runs/v2a/last.pt --net ~/regicide-nnue-v2/v2a.nnue --n 100000
```

Notes:
- **Epochs and lr:** keep 40 × 100M and lr 1e-3. n4b showed little from 80 epochs at v1, and AdamW's per-weight
  steps already compensate for each bucket seeing about 1/8 of the batches. If v2a's `valid(int)` is still falling
  at epoch 40 (n4c ended at 0.514355 on the round-4 set), a 60-epoch run is the next thing to try.
- **Time:** probably 25–40 min per run on one lane. Round 4 was about 22 min for FT 1024, and v2's 8-bucket L1 is
  more GPU work. Two lanes can run side by side as in round 4.
- **Comparing:**
  - Score losses with round 4's `compare_loss.py` on the same validation set. Any build of this branch scores v1 and
    v2 nets with `regicide evaldata`, so no width builds are needed.
  - Match each candidate against n4c with **one** `libregicide_cb.so` from this branch: the built-in n4c on one side,
    `set evalfile v2a.nnue` on the other. Use the usual 1610 games at 0.1 s/move.
  - Check nps first: `printf 'set evalfile v2a.nnue\nbench\nquit\n' | build/regicide`.
- QAT simulates the integer sums in float32, which is exact only up to 2^24. With 2×1024 inputs, a very dense
  position can exceed that, so `valid(int)` might be off in the last digits. The exported net and `check_export`
  are exact regardless (numpy int64).

## Files

- `scripts/train_v2.sh`: one training run in WSL, then the export check and a bench.
- `logs/bench_rounds.txt`: the nps table. `logs/nnuebench.txt`: per-stage timings. `logs/v1_vs_master.txt`: v1
  paired with master, before (−1%) and after (−0.3%) 7aac617.
