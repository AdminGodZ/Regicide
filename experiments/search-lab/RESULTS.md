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
| B3 corr_hist=100 | 617 / 371 / 612 | 23.2% | +1.1 [-4.8, +7.0] | 0 93 609 98 0 | 800 | ±5.9 | +0.37 | 199.2 / 199.5 ms, d 15.30 / 15.27 |
| B4 nosafe_ext=1 | 620 / 361 / 619 | 22.6% | +0.2 [-5.8, +6.2] | 0 98 604 97 1 | 800 | ±6.0 | +0.07 | 200.7 / 200.2 ms, d 15.19 / 15.26 |
| B5 se_double=30 (cap 3) | 623 / 357 / 620 | 22.3% | +0.7 [-5.1, +6.4] | 0 90 617 93 0 | 800 | ±5.8 | +0.24 | 200.2 / 199.6 ms, d 14.91 / 15.24 |
| B5 se_neg=1 | 623 / 356 / 621 | 22.2% | +0.4 [-5.3, +6.2] | 1 87 621 91 0 | 800 | ±5.8 | +0.14 | 198.9 / 199.6 ms, d 15.98 / 15.28 |
| B7 tt_eval=1 | 605 / 381 / 614 | 23.8% | -2.0 [-7.9, +4.0] | 0 101 607 92 0 | 800 | ±6.0 | -0.66 | 199.2 / 198.6 ms, d 15.29 / 15.26 |
| B6 tm_nodes=178 (--target 300) ✓ | 639 / 345 / 616 | 21.6% | +5.0 [+0.4, +9.5] | 0 46 685 69 0 | 800 | ±4.5 | +2.15 | 142.4 / 139.6 ms, d 14.51 / 14.52 |
| B8 lmr_base=80 (1,600 pairs) — **void**, see below | 1231 / 740 / 1229 | 23.1% | +0.2 [-3.9, +4.4] | 0 188 1223 188 1 | 1600 | ±5.9 | +0.09 | 199.5 / 198.8 ms, d 15.45 / 15.40 |

- **B8 is void:** until master PR #31 the LMR table was one process-wide global, refilled by every `Engine::go()`, so in
  an in-process A/B test both sides used whichever lmr_base was set last. The same applies to every earlier
  lmr_base / lmr_mult self-play result (tune1, tune2). Redone in the LMR sweep below.
- The "300 ms" runs above averaged ~200 ms per move: before master PR #37, `--movetime` still stopped early on a stable
  best move. They are internally fair (both sides used the same rule) but not time-comparable with later runs.

B6 time check at `--target 1000` (60 pairs each): tm_nodes 176 → A 446.7 ms vs B 453.2 ms (0.986); 168 → 423.8 vs
445.0 (0.952). Equal time at ≈ 179, used for the confirmation.

### B6 confirmation (`--target 1000`, 400 pairs, tm_nodes=179)

| test | W / D / L | draws | Elo [95% CI] | penta | pairs | ± per 800 | t | time A / B |
|---|---|---|---|---|---|---|---|---|
| B6 tm_nodes=179 (--target 1000) | 319 / 169 / 312 | 21.1% | +3.0 [-2.9, +9.0] | 0 21 351 28 0 | 400 | ±4.2 | +0.99 | 460.2 / 451.8 ms, d 17.04 / 17.04 |

**B6 not confirmed** (lower bound below 0), and A still used 1.9% more time than B. Closed; defaults stay.

### Part B outcome

No idea passed both stages. B1, B2, B3, B4, B5 (double and negative), B7 are neutral (all within ±3.5 Elo); B6 passed
the screen at +5.0 [+0.4, +9.5] (with +2% time) but not the 1000 ms confirmation; B8 is void. With no winner there is
nothing to combine, so no combined test was run. All switches stay in the code, off by default.

## Part C matches: 4 threads vs 4 threads of master

`--tha 4 --thb 4 --threads 4 --hash 64` (4 games at once = 16 search threads), 300 ms, new set, database on. B =
defaults (= master's Lazy SMP: depth skipping, result from thread 0).

| test | W / D / L | draws | Elo [95% CI] | penta | pairs | ± per 800 | t | time A / B |
|---|---|---|---|---|---|---|---|---|
| C1 smp_vote=1 (deepest thread) | 651 / 308 / 641 | 19.2% | +2.2 [-2.9, +7.2] | 0 65 660 75 0 | 800 | ±5.0 | +0.85 | 205.1 / 205.7 ms, d 17.73 / 17.23 |
| C2 smp_vote=2 (Stockfish-style vote) | 640 / 309 / 651 | 19.3% | -2.4 [-7.3, +2.6] | 0 73 665 62 0 | 800 | ±5.0 | -0.95 | 205.7 / 205.7 ms, d 17.74 / 17.19 |
| C3 smp_skip=2,smp_vote=2 | 313 / 162 / 325 | 20.2% | -5.2 [-12.0, +1.6] | 0 38 336 26 0 | 400 | ±4.8 | -1.50 | 200.1 / 205.2 ms, d 17.60 / 17.23 |

(The "d" column for A counts the selected thread's depth, so it is higher than B's by construction.)

**Part C outcome:** picking the result from another thread (deepest: +2.2; voted: −2.4) or letting odd helpers search a
ply deeper (−5.2) does not beat master's Lazy SMP at 4 threads. The scaling limit is search overhead (≈ +40% nodes to
the same depth at 4 threads) and, on this hybrid CPU, threads landing on E-cores. Closed for now by the lead (on
Windows, 4 threads plus pondering gave no gain against Cake in 1,248 games); nothing new started.

# Phase 2 (2026-11-10, after merging master 456e9b2)

- Merged origin/master 456e9b2: with every lab switch at its default, `regicide bench` = **7,677,736** and ctest 27/27.
- Review fixes (01b02b0): `posbench --ttd` takes time and nodes from the completed iteration (the stop was noticed
  up to 1,024 nodes late); `set_evaluator` clears the leaf eval cache (and the correction-history tables, which also
  depend on the evaluator); `smp_vote` no longer vetoes a helper when the main thread has no completed iteration, and
  returns the completed snapshot also when the main thread is chosen.

## Net matches (for truth-data)

**Recipe (as `experiments/truth-data/scripts/jobs/common.sh`, `selfplay_match`):** `tools/match` + `cbhost`, both
sides one `libregicide_cb.so` built from **master 456e9b2** (clang 23.1.4 Release, bench 7,677,736), A and B with
`set threads 1`, `set hashsize 128`, `set book 0`, nets via `set evalfile` (B without evalfile = built-in n6c),
`--openings experiments/eval-gen1/openings4.txt` (805 × 2), CheckerBoard time mode (`--movetime 0.5`, banked),
`--pdn-comments`. Pooled with truth-data's `pool_dirs.py`; per-side time and depth from the PDN comments
(`netm/stats.py`). Scripts in `netm/`.

**Differences from the Windows runs:**
- **Database: 2–7 pieces** (theirs 2–8). The engine finds it through an `egdb` link next to the library (`get
  dbpieces` → 7). The "database off" engine is the same library in a directory without one (`get dbpieces` → 0).
- **Concurrency:** openings4 split into 6 chunks (139/139/134/131/132/130 openings); each chunk is one `tools/match`
  with **`-c 1`**, pinned with `taskset` to one physical P-core (logical CPUs 2k, 2k+1). That makes 6 games at once,
  each alone on its own P-core with both of its engines on that core. E-cores and hyper-thread doubling are not used
  for net games. Nothing else runs on the P-cores.
- Linux laptop (i9-13900H, P-cores up to 5.2/5.4 GHz) instead of the Windows box: absolute depths differ.

**Chunks and completeness (after review):** `netm/chunks/op4_a..f` are `split -n l/6` of openings4;
`netm/check_chunks.py` verifies that they are a partition (concatenated they equal openings4 line for line, 805
distinct openings, each exactly once; sha256 sums in `netm/chunks/CHECK.txt`). Runner v2 (`netm/runner2.sh`) counts a
match as done only if `netm/validate.py --pooled` passes: every chunk's summary says "Games: N of N" without
"(interrupted)", N = 2 × its openings, results.csv has N games, every worker and the pooling step exited 0, and the
pool has exactly 805 pairs. Incomplete chunks are deleted and re-run. The first runner (M5) did not record exit
statuses; its chunks carry an explicit `unrecorded` marker, written only for chunks whose summary shows every game
played, and the validator prints that.

**Intervals:** Elo and its 95% interval in the net-match table come from truth-data's `pool_dirs.py` (pairs, n−1
variance, symmetric interval in Elo via the delta method). `tools/match`'s own summaries (population variance,
transformed endpoints) differ slightly.

Depth columns: mean over the first 20 searched moves (depth ≥ 2) of each side per game. Database endgames reach
very high depths, so the mean over all moves is also given in each match's `stats.txt`.

LMR self-check (step 3 preparation; `selfplay` of this branch, 100 ms, 20 pairs, UHO set): lmr_base=20 vs
lmr_base=120 → mean depth **13.66 vs 14.70** with the per-engine table. With the old `selfplay-lab2` (shared global
table) the same match gave 13.03 vs 13.30. The tool now gives each side its own table.

All at 0.5 s/move (CheckerBoard banked), 805 pairs, master 456e9b2 for both sides. Per-game files (results.csv,
gzipped PDN with per-move comments, summary per chunk) in `netm/matches/<match>/`.

| match | database | W / D / L (A) | pentanomial [LL LD DD/WL WD WW] | Elo [95% CI] | LOS | mean time per move A / B | mean depth A / B | forfeits / illegal moves |
|---|---|---|---|---|---|---|---|---|
