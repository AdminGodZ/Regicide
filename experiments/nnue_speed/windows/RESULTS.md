# nnue_speed on x86: verification on the Core Ultra 9 285K (AVX-VNNI) and matches with the new code

Branch `t3/nnue-speed` @ `e6a8e3f`, checked on the Windows box (Intel Core Ultra 9 285K, WSL2 Ubuntu, clang 22.1.8).
This is the first machine to run the AVX-VNNI kernel. `-march=native` defines `__AVX2__ __AVXVNNI__`; there is no
AVX-512.

Before the verification, datagen (gen3d) was paused (SIGSTOP), and it stayed paused through all the matches
(08:36 to 09:36). The machine was otherwise idle.

## 1. `verify_x86.sh` (all checks pass)

Command: `J=16 experiments/nnue_speed/scripts/verify_x86.sh <slice>`.
- The slice is a 100,000-record DataRecord file: the first 100k positions of the NNUE round-3 validation set
  (`nnue_valid_1M.bin`, the held-out tails of gen1..gen3b).
- The baseline is `821c89e`, with the widths changed by `sed`.
- Full log: `logs/verify.txt`; bench runs: `logs/bench_runs.txt`, `logs/bench_summary.txt`.

| Check | Result | |
|---|---|---|
| `vpdpbusd` in the native build | 108 instructions (FT 1024 build: 108, L1 64 build: 208) | **PASS** |
| `vpdpbusd` in the `-march=haswell` build | 0 | **PASS** |
| ctest, native build | 100% of 8: perft, eval_builtin, nnue, **nnue_ft1024**, **nnue_l1_64**, cb_api, egdb, egdb_data | **PASS** |
| ctest, haswell build (AVX2 maddubs path, run natively on x86 for the first time) | 100% of 8 | **PASS** |
| evaldata, new vs baseline, 100,000 records | nn2b identical, nn3c identical, nn3d identical | **PASS** |
| built-in bench | 6,873,528 | **PASS** |
| bench nodes, every run (6 rounds x 3 nets x 2 sides) | 6,873,528 / 6,167,330 / 5,185,919 | **PASS** |

The ctests `nnue_ft1024` and `nnue_l1_64` pass in the native build. They compare the VNNI kernels against the naive
reference, so **the AVX-VNNI path is correct on real hardware**. So is the AVX2 path (haswell build).

### nps (bench, 6 alternating rounds, median)

| Net | Width | Baseline 821c89e | New e6a8e3f | Speedup | Share of new FT 512 | Round 3 (825f993) |
|---|---|---|---|---|---|---|
| nn2b | FT 512 | 4.34M | **4.79M** | **+10.3%** | 100% | 100% |
| nn3c | FT 1024 | 3.06M | **3.78M** | **+23.4%** | **79%** | 72% |
| nn3d | L1 64 | 3.44M | **4.30M** | **+25.2%** | **90%** | 83% |

The gains are smaller than on the M5 (+20 / +39 / +32%), but wide nets still gain about twice as much as FT 512.
`nnuebench` stage timings (new code, ns per call):

| Stage | FT 512 | FT 1024 | L1 64 |
|---|---|---|---|
| evaluate | 80.0 | 128.5 | 132.8 |
| transform (SCReLU) | 22.4 | 59.4 | 22.5 |
| FT → L1 | 49.2 | 81.4 | 96.7 |
| L1 → L2 | 4.2 | 4.2 | 7.4 |
| update, one move, both views | 67.9 | 156.0 | 63.5 |
| refresh, both views | 162.2 | 313.0 | 159.5 |
| non-zero FT chunks | 76.5% | 65.2% | 77.4% |

On x86 the FT 1024 transform (59 ns) costs almost as much as the FT → L1 kernel. The AVX2 transform is not fused
(the README lists this as a next step). On x86 it is a larger share than on NEON.

### FT 768 (nn3e), extra check

nn3e is an FT 768 net trained in NNUE round 3 (`experiments/nnue3`, addendum on `t3/nnue3`). The script does not
cover this width, so it was checked separately (`logs/checks_768.txt`):
- The new code built with `-DREGICIDE_NNUE_FT=768` passes ctest (7/7).
- `evaldata` on the same 100k records is **identical** to `821c89e` with `FT = 768`.
- bench with nn3e is 7,245,436 nodes on both sides.
- nps (3 rounds): 3.31 / 3.52 / 3.46M before vs 3.73 / 4.01 / 4.06M after, **+16%** (medians 3.46 → 4.01M). That is
  84% of new FT 512 (measured in another session).

`logs/evaldata_sha256.txt` has checksums of all evaldata outputs.

## 2. Matches with the new code

The settings are those of `experiments/nnue3`:
- 805 4-ply openings x 2 colours (`openings4.txt`, 1610 games), `--movetime 0.1`, `-c 10`.
- `set threads 1`, `set hashsize 128`, `set book 0`, 2-7 piece endgame database.
- Datagen paused.

Engines are native Linux `regicide_cb.so` builds of `e6a8e3f`: FT 512 (default), `-DREGICIDE_NNUE_FT=1024`,
`-DREGICIDE_NNUE_L1=64`, `-DREGICIDE_NNUE_FT=768`. Each contains `vpdpbusd`.
- In (a), "old" is the native Linux build of master `ae210fb`, which has no `vpdpbusd`. This is the commit the
  branch's search is based on: `821c89e` = `ae210fb` + the nnue3 merge, with the same engine code.
- In (b) to (d), A loads its net with `set evalfile`, and B is a separate copy of the new FT 512 `.so` with its
  built-in nn2b.

| Match | Elo [95% CI] (pentanomial) | LOS | W / D / L | pairs LL LD DD+WL WD WW |
|---|---|---|---|---|
| (a) **new + nn2b vs old ae210fb + nn2b** | **−1.5** [−5.1, +2.1] | 20.30% | 285 / 1033 / 292 | 0 / 39 / 734 / 32 / 0 |
| (b) **nn3c (FT 1024) vs nn2b**, both new | **+2.2** [−2.6, +6.9] | 81.17% | 287 / 1046 / 277 | 1 / 56 / 680 / 68 / 0 |
| (c) **nn3d (L1 64) vs nn2b**, both new | **+1.7** [−2.9, +6.4] | 76.75% | 294 / 1030 / 286 | 0 / 56 / 685 / 64 / 0 |
| (d) **nn3e (FT 768) vs nn2b**, both new | **+5.4** [+0.9, +9.9] | 99.09% | 295 / 1045 / 270 | 0 / 44 / 692 / 69 / 0 |

Times: (a) 08:40 to 08:53, (b) to 09:08, (c) to 09:22, (d) to 09:36. Average time per move was 0.048-0.051 s for
every engine.

The same nets on the old code (round 3, 825f993 search):

| Net | Old code | New code | Change |
|---|---|---|---|
| nn3c (FT 1024) | +0.9 [−4.1, +5.9] | +2.2 [−2.6, +6.9] | +1.3 |
| nn3d (L1 64) | −0.0 [−5.2, +5.2] | +1.7 [−2.9, +6.4] | +1.7 |
| nn3e (FT 768) | +0.2 [−4.9, +5.4] | **+5.4** [+0.9, +9.9] | +5.2 |

### Reading

- **(a) The pure speed gain does not show in play at 0.1 s/move.** The new code is −1.5 [−5.1, +2.1], so not
  significant either way. So +10% nps is worth at most about +2 Elo here. That is plausible for this match: draws are
  64%, and only 71 of 805 pairs are decisive. Speed-only changes of this size need far more games, or a longer time
  control, to measure. Correctness is not in question, since every eval and bench node count is identical.
- **(b)-(d) All three wider nets now score positive against nn2b.** Before, they were at or near zero.
  - Only **nn3e (FT 768)** is significant: +5.4, CI lower bound +0.9, LOS 99.1%.
  - It is a single 1610-game match, though, and the old-code result (+0.2) is within its CI. Part of the +5.2 change
    is probably noise.
  - nn3c and nn3d improve by about as much as their extra speed would suggest (+23% / +25% vs +10% for nn2b).
- **Recommendation:** nn3e on the new code is the first candidate that meets the "CI lower bound > 0" bar. Before it
  becomes the built-in net, a confirmation would be worth having: a second match at a longer time control, e.g.
  0.3 s/move. Wide nets should gain from more time per move, and it would show whether +5.4 holds up.

## Files

- `logs/`:
  - `verify.txt`: the full output of `verify_x86.sh`;
  - `bench_runs.txt`, `bench_summary.txt`;
  - `checks_768.txt`: the FT 768 checks and the bench of the match builds;
  - `evaldata_sha256.txt`;
  - `status.txt`: pipeline timestamps.
- `matches/<A>-vs-<B>/`: `summary.txt` and `results.csv` per match.
- `scripts/`:
  - `nsv_verify.sh`: pauses datagen, then runs `verify_x86.sh`;
  - `nsv_matches.sh`: the match builds, the FT 768 check and the four matches. It was started only after the
    verification log had been checked.
