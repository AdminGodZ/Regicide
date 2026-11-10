# search-lab: more sensitive tests, untried search ideas, thread scaling

- **Base:** master fcf6acb (built-in n6c, se_depth 4, se_margin 20); branch t3/search-lab. Engine source = 6bb8c6c
  (tune2's base) apart from the embedded book, which selfplay never loads.
- **Laptop:** i9-13900H, Arch, clang 23.1.4 Release; jobs are systemd user services (`regicide-searchlab`), nice 10,
  sleep inhibited. Runner and queue in `~/regicide-jobs/searchlab/` (copy of the runner here).
- **Switches** (all `SearchParams`, off by default; `regicide bench` = 7,712,020 with everything off, unchanged):

| switch | idea |
|---|---|
| `hash_first_db=D` | B1: probe the hash table before the endgame database; database results are stored as exact at depth D |
| `leaf_cache=n` / `leaf_tt=1` | B2: per-thread 2^n eval cache / store horizon evals in the hash table (no bound) |
| `corr_hist=w` | B3: correction history (own men, opponent men, kings+material hashes, per side to move), weight w % |
| `nosafe_ext=n` | B4: horizon extension when at most one move avoids handing over a capture (n per line) |
| `se_double=m`, `se_double_cap=c`, `se_neg=1` | B5: double extension (alternatives below sb − m, at most c per line, non-PV); negative extension |
| `tm_nodes=b` | B6: soft-limit factor (b − 100 × best-move node share)/100 in --target / clock modes |
| `tt_eval=1` | B7: hash score as the pruning eval when its bound allows |
| `smp_vote=1/2`, `smp_skip=1/2` | C: result from the deepest / SF-voted thread; no depth skipping / odd helpers one ply deeper |

- **Tools:** `tools/unbalanced.cpp` (unbalanced opening generator), `selfplay --book <file>` (FEN per line) and per-side
  mean time/depth per move, `tools/posbench.cpp` (fixed searches over positions: nps, depth, time to depth).

## Part A: unbalanced opening set

`books/uho1600.txt`: 1,600 positions. Each: a random 3-ply opening + 2–24 plies (5k-node best move, or a random legal
move with 20% probability); kept if ≥ 9 pieces, no capture for the side to move, new, and a 2M-node search (after a
100k-node prefilter) scores it 50–130 for one side. 24,146 candidates tried. Pieces: 10–24 (median 18); |score|
quartiles 67 / 88 / 110; side to move B 814 / W 786; favoured side W 1,028 / B 572.

### Calibration: two known differences, old set vs new set

`selfplay --movetime 300 --threads 16 --pairs 800 --egdb <2-7 piece db>` (1 search thread per engine), binary
`selfplay-lab2` (all switches off). Old set = the default 3-ply set (216 positions); new = `--book books/uho1600.txt`
(800 of the 1,600 positions, shuffled, each with both colours). n4c = `experiments/nnue4/n4c.nnue` as `--evalb`.
"± per 800 pairs" is the half-width of the 95% interval; t = Elo / standard error.

| test | set | W / D / L (A) | draws | Elo [95% CI] | penta [LL LD DD+WL WD WW] | ± per 800 pairs | t | wall time |
|---|---|---|---|---|---|---|---|---|
| se_margin 30 vs 20 | **new** | 604 / 348 / 648 | 21.8% | **−9.6 [−15.3, −3.8]** | 0 114 616 70 0 | ±5.8 | **−3.27** | 21 min |
| se_margin 30 vs 20 | old | 248 / 1093 / 259 | 68.3% | −2.4 [−6.8, +2.0] | 0 59 693 48 0 | ±4.4 | −1.07 | 18 min |
| n6c vs n4c | **new** | 635 / 370 / 595 | 23.1% | **+8.7 [+2.9, +14.5]** | 0 73 614 113 0 | ±5.8 | **+2.94** | 21 min |
| n6c vs n4c | old | 287 / 1056 / 257 | 66.0% | +6.5 [+1.6, +11.5] | 0 53 664 83 0 | ±5.0 | +2.57 | 18 min |

References without the database: tune2 measured se_margin 30 at −6.5 [−11.5, −1.5] (old set, 300 ms, t ≈ −2.6), and
nnue6 measured n6c vs n4c at +4.3 [+0.3, +8.4] (CheckerBoard harness, 100 ms, 1,610 games).

- **Draws fall from ~67% to ~22%.** The error bar per 800 pairs gets a little wider (±5.8 vs ±4.4–5.0), because almost
  every pair is now one win each (WL), and only LD/WD pairs move the score. But the measured differences grow more:
  ×4 for se_margin (−9.6 vs −2.4) and ×1.3 for the net.
- **Games needed for the same confidence scale with 1/t².** se_margin: (3.27/1.07)² ≈ 9× fewer games on the new set;
  the net: (2.94/2.57)² ≈ 1.3×. Pooled over both (Σt² 19.3 vs 7.7), **~2.5× fewer games**, ~2.1× less wall time (the
  new set's games last a little longer). With one run per cell the t values themselves are uncertain (±1 each), so the
  factor could be anywhere from ~1.3× to ~9×. In both tests, though, the new set gives the larger t, and the old set
  with the database does not resolve se_margin 30 at all.
- In every test both LL and WW are 0 (or nearly): neither engine wins a pair outright.
- **Decision: Part B uses the new set** (`--book books/uho1600.txt`), at 300 ms × 800 pairs with the database.
  Elo values on this set are inflated compared with the old set, so they are not directly comparable with tune1/tune2
  numbers. The screening bar (≥ +5 and lower bound > 0) is kept as briefed. Since the set amplifies differences, it is
  if anything easier to pass on this set.

## nps measurements (B1, B2)

`posbench`, 1 thread, 300 ms per position, 16 MB hash (as in selfplay), 2–7 piece database, idle laptop, two passes
each (`logs/nps_b1b2.txt`, `logs/nps_b2_mid.txt`). Positions: `books/end8_11.txt` (100 positions with 8–11 pieces from
5k-node walks of 40–90 plies) and `books/mid50.txt` (60 middlegame positions, 14–20 pieces).

| switch | 8–11 pieces: nps (pass 1 / 2) | vs base | db probes | middlegame nps | vs base |
|---|---|---|---|---|---|
| base | 3,724,688 / 3,710,647 | — | 8.13M | 3,081,375 / 3,077,602 | — |
| hash_first_db=90 | 3,728,627 / 3,692,819 | −0.2% | 4.31M (−47%) | | |
| hash_first_db=1 | 3,721,341 / 3,711,457 | −0.0% | 4.29M | | |
| leaf_cache=16 | 4,055,343 / 4,063,675 | **+9.1%** | | 3,216,363 / 3,233,201 | **+4.8%** |
| leaf_cache=18 | | | | 3,247,185 / 3,255,924 | +5.6% |
| leaf_tt=1 | 4,032,816 / 4,042,352 | +8.6% | | 3,194,656 / 3,175,747 | +3.5% |

- B1 halves the database probes but does not change nps: a probe of the in-memory 2–7 piece database costs about as
  much as a hash probe. The stored depth (90 or 1) makes no difference; 90 is screened.
- B2: `leaf_cache=16` (512 KB per thread) is screened: nearly as fast as 2^18 and kinder to shared caches when 32
  engines run at once.

## Part C diagnostics: Lazy SMP scaling

`posbench --ttd 20`: the search has no depth limit and stops when the main thread completes depth 20 (a plain depth
limit is misleading: helpers that skip depths reach the limit early and then idle). 50 middlegame positions
(`books/mid50.txt`), 64 MB hash, database on, idle laptop. Speedup = geometric mean over positions of
time(1 thread) / time(n threads). Files in `logs/ttd/`.

Unpinned (the OS places the threads on P or E cores):

| threads | TTD speedup (default skip) | no skip (`smp_skip=1`) | odd helpers +1 ply (`smp_skip=2`) | nps vs 1 thread (default) |
|---|---|---|---|---|
| 2 | 1.59 | 1.47 | 1.60 | ×2.0 |
| 4 | 2.79 | 1.94 | 1.88 | ×3.9 |
| 8 | 2.20 | 2.16 | 2.09 | ×4.0–4.5 |

Pinned to separate physical P-cores (`taskset -c 0,2,4,6`; 1 thread on CPU 0), repeated:

| config | TTD speedup per run | total nodes to depth 20 vs 1 thread |
|---|---|---|
| 2 threads | 1.52, 1.52 | ×1.28, ×1.37 |
| 4 threads, default skip | 2.99, 2.41, 2.20 | ×1.44, ×1.29, ×1.56 |
| 4 threads, no skip | 2.01, 1.90, 1.94 | ×1.59, ×1.62, ×1.70 |
| 4 threads, odd +1 ply | 1.84, 1.95, 1.90 | ×1.60, ×1.56, ×1.61 |

What limits the scaling:
- **Search overhead, not nps.** Pinned 4 threads run at ~×3.6 the nodes per second of one, but need ~40% more nodes to
  reach the same depth, so the time-to-depth gain is ~×2.5. At 2 threads the overhead is ~30% (×1.52).
- **8 threads are no better than 4 on this CPU:** the extra threads land on E-cores (4.1 GHz max, slower per clock)
  or on hyper-thread siblings; nps grows only from ×3.9 to ×4–4.5 while overhead grows to ~×1.9 nodes.
- **Core placement noise:** unpinned runs of the same configuration differ by up to 25% in nps (8.5M vs 11.5M at 4
  threads), depending on whether the main thread lands on an E-core. I first suspected hash-table cache-line contention,
  but a variant that skips rewriting unchanged entries (`tt_skip_same=1`) showed nothing beyond this noise.
- **The depth-skip pattern is the best of the three tried**: without skipping, or with odd helpers one ply deeper, the
  helpers duplicate more of the main thread's work (×1.6–1.7 nodes) and time-to-depth falls to ~×1.9.
- The result always comes from thread 0 (`smp_vote` tests this in play).

## Part B: screens (new set, 300 ms × 800 pairs, 2–7 piece database)

A = the switch on, B = defaults; `selfplay-lab2 --movetime 300 --threads 16 --book books/uho1600.txt --egdb …`.
✓ = ≥ +5 Elo with the CI lower bound > 0 (goes to the 1000 ms × 400 pairs confirmation). Columns: draws, Elo [95% CI],
pentanomial, pairs, ± scaled to 800 pairs, t, mean time / depth per move (A / B).

B6 is played in `--target 300` mode (no `--movetime`), with tm_nodes set for equal average time per move. Short
calibration runs (60 pairs each): tm_nodes 150 → A 104.8 ms vs B 138.3 ms; 170 → 130.3 vs 140.8; 190 → 154.5 vs
138.8. Interpolated equal time at ≈ 178.

| test | W / D / L | draws | Elo [95% CI] | penta | pairs | ± per 800 | t | time A / B |
|---|---|---|---|---|---|---|---|---|
| B1 hash_first_db=90 | 607 / 370 / 623 | 23.1% | -3.5 [-8.2, +1.3] | 0 71 674 55 0 | 800 | ±4.8 | -1.44 | 199.8 / 199.7 ms, d 15.24 / 15.27 |
| B2 leaf_cache=16 | 618 / 363 / 619 | 22.7% | -0.2 [-5.1, +4.7] | 0 66 669 65 0 | 800 | ±4.9 | -0.08 | 199.5 / 199.2 ms, d 15.46 / 15.30 |
