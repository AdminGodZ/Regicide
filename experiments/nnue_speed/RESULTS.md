# nnue_speed: faster NNUE inference, especially for wider nets

Branch `t3/nnue-speed`, from `t3/optimize-checkers-engine` @ `821c89e`. Goal: make FT 1024 (nn3c) and L1 64 (nn3d)
cheap enough that their lower loss turns into Elo (round 3: 72% and 83% of nn2b's nps on the 285K, +0.9 and −0.0 Elo).

**Every change is bit-exact.** The built-in nn2b still benches **6,873,528** nodes. nn3c and nn3d bench the same
node counts before and after (6,167,330 and 5,185,919). The `nnue` ctest (forward pass vs a naive reference,
lazy incremental vs refresh, colour symmetry) passes for FT 512, FT 1024 and L1 64:
- natively (NEON sdot);
- under Rosetta (x86_64 `-march=haswell`, AVX2 maddubs);
- on a portable arm64 build (`-march=armv8-a`, no dotprod).

The AVX-VNNI path compiles (`-march=arrowlake` emits `vpdpbusd`) but has **not run anywhere yet**; see
[Windows verification](#windows-verification).

## Commits

| Commit | Change |
|---|---|
| `69eb818` | `regicide nnuebench <net\|builtin> [data.bin] [count]`: per-stage timings and activation sparsity (`Network::profile`). CMake `REGICIDE_NNUE_FT` / `REGICIDE_NNUE_L1` build other widths without patching. The ctests `nnue_ft1024` and `nnue_l1_64` run the nnue test at those widths in the default build. |
| `806e1a3` | Sparse FT → L1. SIMD builds store the hidden-layer weights in chunk order (permuted at load, so RGNN is unchanged). `find_nnz` lists the non-zero 4-byte chunks of the SCReLU output, and only those are multiplied, broadcast against 4 (NEON `vdotq_s32`) or 8 (AVX2) outputs per instruction. On x86 the instruction is `_mm256_dpbusd_avx_epi32` under `__AVXVNNI__`, `_mm256_dpbusd_epi32` under AVX-512 VNNI+VL, and otherwise `maddubs`+`madd` (exact: inputs ≤ 127, so pair sums stay < 2^15). L1 → L2 uses the same kernel, dense. The portable path is unchanged. |
| `ccd824c` | Lazy accumulators per point of view: `get()` updates only the side to move's view. Leaves never compute the other one. |
| `6c999bf` | FT outputs reordered at load time so that zeros share chunks: SCReLU statistics over 4096 random playouts (fixed seed), then greedy grouping. A permutation of integer sums, so exact. Loading takes about 25 ms (FT 512) / 33 ms (FT 1024). |
| `f8abdcd` | A chain of lazy updates is applied in one pass, tile by tile (128 int16 kept in registers), storing every intermediate. This replaces the three special cases of the old `update`. |
| `fd0f8b8` | NEON SCReLU with the chunk search fused in (saturating narrows, `vmull_u8`, a multiply to gather the mask): about 30% fewer instructions. |
| `965b2eb` | tools/nnue/README: FT 512 default, lazy views, configure-time widths. |

## Results on the Mac (M5 Pro, Apple clang 21, NEON + dotprod)

Real nets throughout: nn2b (built-in, FT 512), nn3c (FT 1024 build), nn3d (L1 64 build). The positions are the first
1024 records of a tiny datagen run (`datagen --games 150 --nodes 3000 --threads 2 --seed 5`, 15,455 positions).
"Before" is `69eb818`, which has the old kernels plus the profiler. Raw output is in `logs/mac_profile.txt` and
`logs/mac_bench.txt`.

### Per component (ns per call, `regicide nnuebench`)

| Stage | FT 512 before → after | FT 1024 before → after | L1 64 before → after |
|---|---|---|---|
| evaluate (total) | 134 → **113** (−16%) | 278 → **186** (−33%) | 248 → **181** (−27%) |
| transform (SCReLU; after: + chunk list) | 14.5 → 20.4 | 32.8 → 45.0 | 14.7 → 20.7 |
| FT → L1 | 108 → **79** (1.37x) | 238 → **139** (1.71x) | 216 → **140** (1.54x) |
| L1 → L2 | 7.2 → 6.3 | 7.2 → 6.4 | 13.3 → 12.0 |
| output | 1.9 → 2.0 | 1.9 → 2.0 | 1.9 → 2.0 |
| update, one move, both views | 49 → 50 | 106 → 107 | 49 → 50 |
| refresh, both views | 213 → 213 | 427 → 428 | 214 → 214 |
| non-zero FT chunks | 82.6% → 75.8% | 72.1% → 64.8% | 84.5% → 77.1% |

Typical sparsity on real positions:
- Only 36% (FT 512) and 28% (FT 1024) of the SCReLU outputs are non-zero.
- But 83% and 72% of the 4-byte chunks are, so skipping chunks saves much less than the byte sparsity suggests. The
  reordering brings this to 76% / 65%. The same greedy grouping fitted on real games reaches about 70% / 61% on held-out
  positions, so random playouts leave a few points.

The microbenchmark times one isolated update per call, so the per-view laziness and chain fusion show only in
`bench`. With per-view laziness, the built-in bench applies 2.27 single-view moves per evaluation. 56% of them come
after the first move of a chain (most chains are 2 moves: the view was last computed at the grandparent), and the
fused pass skips reloading the accumulator for those.

### Bench (`set evalfile <net>`, `bench`, 4 alternating rounds, median nps)

| Net | Nodes (same before/after) | Before | After | Speedup | vs FT 512 after |
|---|---|---|---|---|---|
| nn2b, FT 512 | 6,873,528 | 5.17M | **6.20M** | **+20%** | 100% |
| nn3c, FT 1024 | 6,167,330 | 3.17M | **4.42M** | **+39%** | 71% |
| nn3d, L1 64 | 5,185,919 | 3.97M | **5.23M** | **+32%** | 84% |

How each step added up for FT 512 (single sessions, roughly ±1%):
- kernels 5.18 → 5.45M;
- per-view laziness → 5.80M;
- reordering → 6.14M;
- chain update → ~6.15M (FT 1024 +1%);
- NEON transform → 6.20M (FT 1024 +3.5% for the last two together).

FT 1024 now runs at 85% of the *old* FT 512 speed, and L1 64 at 101%.

Where the search time goes now (`sample`, bench 17):
- FT 1024: accumulator updates about 30%, FT → L1 about 28%, transform about 8%, the rest search, TT and move
  generation.
- FT 512: FT → L1 about 22%, updates about 24%.

On the M5 the FT → L1 kernel is bound by loading the weights: about 128 bytes per non-zero chunk, roughly 40 B per cycle.
So on this machine only fewer chunks make it faster.

### Under Rosetta (x86_64, `-march=haswell`, AVX2 maddubs path)

`nnue`, `nnue_ft1024` and `nnue_l1_64` pass. The bench node counts match NEON:
- built-in: 6,873,528;
- nn3c: 6,167,330;
- nn3d: 5,185,919.

Rosetta nps (1.1-1.7M) says nothing about the real x86 speed.

## Windows verification

Not checked yet, and only the 285K can check it:
- **The AVX-VNNI kernel has never run.** Its correctness rests on the semantics of `vpdpbusd`: unsigned activations
  0..127 times int8 weights, 4 products added into int32 with no saturation, the same sum as the other paths.
- The AVX2 path has only run under Rosetta.
- x86 nps. On the 285K the old kernel spent 3 instructions per 32 weight bytes (`maddubs`, `madd`, `add`); VNNI
  needs 1, so FT → L1 should gain more there than on the Mac.

Run in WSL2 on the 285K, from the root of a `t3/nnue-speed` checkout, with datagen paused:

```bash
git fetch origin && git checkout t3/nnue-speed
experiments/nnue_speed/scripts/verify_x86.sh [path/to/validation_or_slice.bin]
```

What it does (log in `experiments/nnue_speed/logs/x86_<date>/verify.txt`):
1. Builds the new code with `-march=native`: FT 512, `-DREGICIDE_NNUE_FT=1024` and `-DREGICIDE_NNUE_L1=64`.
   Also builds an `-march=haswell` copy for the AVX2 path. Prints the compiler's `__AVXVNNI__` / `__AVX2__` macros.
   - **Check:** `vpdpbusd` count > 0 in the native build and 0 in the haswell build.
2. Builds the baseline `821c89e` in three worktrees (`../regicide-nsv-base-{512,1024,l164}`). The width is changed
   by `sed`, as in nnue3.
3. Runs ctest on the native and haswell builds.
   - **Check:** 100% pass, including `nnue_ft1024` and `nnue_l1_64`. These test the VNNI kernels against the naive
     reference at both widths.
4. Runs `evaldata` on 100,000 records for nn2b, nn3c and nn3d, new vs baseline.
   - **Check:** identical for all three.
5. Runs `bench`.
   - **Check:** the built-in net gives 6,873,528.
6. Runs 6 alternating rounds of bench, baseline vs new, for each of the three nets.
   - **Check:** nodes 6,873,528 / 6,167,330 / 5,185,919 in every run.
   - **Report:** median nps, new vs baseline, and FT 1024 / L1 64 as a share of new FT 512 (round 3 had 72% / 83%).
7. Prints `nnuebench` stage timings for the new code.

If the gains hold, the next step is to repeat the round-3 matches with the new code on both sides:
- nn3c (FT 1024 build) vs nn2b;
- nn3d (L1 64 build) vs nn2b;
- the same settings as `experiments/nnue3` (0.1 s/move, 1610 games).

If CheckerBoard / llvm-mingw builds are used, they also need a bench node check. `-march=native` there must define
`__AVXVNNI__`.

## Possible next steps

- **Reordering from real data.** The exporter could store the FT outputs already grouped, from the training data.
  This is a permutation of the file's FT order, so the format is unchanged; the net bytes change but evaluations
  don't. It would bring the chunk density from 76% to about 70% (FT 512) and from 65% to about 61% (FT 1024), and
  drop the load-time playouts.
- **Accumulator updates** are now the largest NNUE cost at FT 1024 (2 KB rows; 256 KB of FT weights, more than L1D).
- **The AVX2 transform** is the auto-vectorised SCReLU plus a separate `find_nnz` pass. A fused AVX2 version (as on
  NEON) would save one pass over the 1 KB of activations.
