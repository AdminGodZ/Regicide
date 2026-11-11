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

**Strict validation (second review, 2026-11-11):** `netm/validate.py` now checks every planned pair: exactly two
rows with distinct game ids, the planned opening (`op4_<c>:<pair>`), one game per colour (A black / B white and the
reverse), a score consistent with the result and A's colour, no duplicate or unplanned rows. The pool counts
validated pairs only (`--write-pool`, same formula and format as `pool_dirs.py`, which accepted any two rows with the
same pair number). `netpost.sh` runs this check itself and posts from that pool. Re-validated: **M5 and M1 are
unchanged** (805/805 valid pairs each, identical W/D/L, pentanomial and Elo; `netm/matches/M?_*/validate.txt`). A
tampered copy of M1 (a losing row replaced by its pair's winning row; a duplicated row; a draw scored 1) is rejected
in each case. Sweep runner: a line counts as done only on exit 0 with a final report over all queued pairs; failed
attempts keep their log and go to `failed.txt`. `row.py` labels anything shorter "(partial: X of P pairs)".

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
| M5 n8c vs n8bn | on | 262 / 1078 / 270 | [0, 41, 731, 33, 0] | **-1.7 [-5.4, +1.9]** | 17.6% | 0.271 / 0.270 s | 33.75 / 33.84 | none |
| M1 n8q vs n6c (interim at 406 pairs was +3.9 [-1.6, +9.3]) | on | 265 / 1096 / 249 | [0, 23, 743, 39, 0] | **+3.5 [+0.1, +6.8]** | 97.9% | 0.275 / 0.275 s | 33.76 / 33.44 | none |
| M3 n8q vs n6c, database OFF (replayed in full after the reboot stop; the stopped run had 391 games, −6.3 [−14.3, +1.7]) | off | 268 / 1077 / 265 | [0, 38, 726, 41, 0] | **+0.6 [-3.1, +4.4]** | 63.2% | 0.439 / 0.439 s | 20.71 / 20.63 | none |

**M6a w2064 vs n8bn, first attempt (2026-11-11 03:54): failed before any game.** The 456e9b2 build caps the feature
transformer at 1024 and rejected w2064 (FT 2048): `set evalfile` → "ft must be a multiple of 128 up to 1024". The
match tool refused to start (exit 1), and the runner logged 0 of 1,610 games and put the line in `failed.txt`. Re-run
on a second build of the same master 456e9b2 with `-DREGICIDE_NNUE_MAX_FT=2048` (as truth-data's `rft2048.so`).
Both sides use it. bench 7,677,736 (unchanged); it loads w2064 and n8bn; database 7 pieces. Engine dir `netm/eng2048`.
| M6a w2064 vs n8bn (FT-2048 build of 456e9b2, both sides) | on | 262 / 1071 / 277 | [0, 35, 750, 20, 0] | **-3.2 [-6.4, -0.1]** | 2.1% | 0.271 / 0.270 s | 31.96 / 33.38 | none |

**M7 n9h vs n8q (database on, 0.5 s, the M1 protocol):** n9h.nnue from origin/t3/truth-data, sha256 2bae308ed648d2ef….
Engine: both sides one `libregicide_cb.so` built from **master cc29fc9** (PR #60: net loader range check; search
unchanged; bench 7,677,736), dir `netm/eng_cc29` with the 7-piece database. `set evalfile` accepts n9h and n8q
(`reply 1 nnue`) and rejects a missing file. The match tool and cbhost are the same 456e9b2 builds as for M1, so the
protocol is identical. The chain had started M7 at 06:21 on the 456e9b2 engine; that run was stopped at 06:40 after
221 games, moved aside unpooled (`~/regicide-jobs/netm/discarded/`), and M7 restarted in full on cc29fc9 at 06:41.
(The restart first failed to launch: `runner2.sh` had lost its execute bit when it was rewritten for the strict
validation; fixed with chmod.)
| M7 n9h vs n8q (master cc29fc9 engine; interim at 402 pairs was +3.9 [-1.0, +8.7]) | on | 270 / 1084 / 256 | [0, 26, 739, 40, 0] | **+3.0 [-0.4, +6.5]** | 95.8% | 0.269 / 0.269 s | 33.33 / 33.17 | none |

# Experiment Q: forcing quiet moves exempt from shallow pruning (`prune_forcing`)

Base: t3/search-lab with master ddd25fc merged (n8q built in), `regicide bench` 7,833,556 with the switch at 0.
In `Worker::search`, late-move pruning and futility pruning skip a quiet move before its child exists, so they never
see that the move forces the opponent to capture (the usual start of a checkers combination); the capture search
below cannot bring such a move back.
- `prune_forcing=1`: when LMP or futility would skip a quiet move, the child is made first, and the move is not pruned
  if the opponent has a capture there. `2`: such moves are also exempt from LMR (no reduction).
  `-1`: statistics only (same tree as 0).
- Statistics per search (`Engine::prune_stats`, printed by posbench): moves each rule applied to and how many the
  exemption kept.

## Cost at fixed depth

`regicide bench` (fixed depth): 0 → 7,833,556 nodes; 1 → 12,823,262 (×1.64); 2 → 19,596,151 (×2.50); −1 → 7,833,556.

posbench, 50 middlegame positions (`books/mid50.txt`), depth 20, 1 thread, 64 MB hash, database on, all on one
E-core (`q/q_posbench_pf*.txt`):

| prune_forcing | nodes to depth 20 | time to depth 20 | LMP applied / kept | futility applied / kept | LMR applied / exempt |
|---|---|---|---|---|---|
| 0 | 68,484,463 | 105.5 s | 3,968,137 / — | 30,864,623 / — | 7,974,708 / — |
| −1 (stats only) | 68,484,463 | 107.8 s (+2%) | 3,968,137 / *2,374,909 forcing* | 30,864,623 / *16,954,964 forcing* | 7,974,708 / *4,615,806 forcing* |
| 1 | 129,159,582 (×1.89) | 177.2 s (×1.68) | 7,051,660 / 4,609,326 | 40,027,398 / 22,076,310 | 8,594,052 / — |
| 2 | 208,407,530 (×3.04) | 293.0 s (×2.78) | 9,624,647 / 6,347,382 | 65,133,189 / 36,075,413 | 14,286,811 / 9,465,155 |

**60% of the quiet moves LMP prunes and 55% of those futility prunes are forcing** in these middlegames: in checkers a
large share of quiet moves hand the opponent a capture. So the exemption is broad and expensive, not a narrow fix.

## Position test (64 positions from lost games)

**Positions** (`q/q64.tsv`, `q/select.py`): decisive games from Book Lab PDNs on origin/t3/booklab (f1, f4, k3, v11c-n8q,
v11d, v13, w10-b3 runs; replayed with the new `tools/pdnfens`). From each game, the last position with 10–14 pieces
in which the eventual loser is to move and has at least 2 legal moves; for the f1 games, before the ply where Book Lab's
`f1_dbscan_games` table first shows a database decision (4 positions), otherwise the last such position in the game
(60). 32 positions come from Regicide's losses and 32 from the opponents' losses, round-robin over runs, unique FENs.
Each is searched single-threaded from a fresh hash with the 2–7 database at 1M, 20M and 100M nodes with
prune_forcing 0 and 1 (the 100M runs are the references, one per setting; `q/out/`).

| | 1M nodes | 20M nodes | 100M nodes |
|---|---|---|---|
| time for 64 positions (E-core), pf0 / pf1 | 108 / 98 s | 2,036 / 1,861 s | 9,146 / 5,724 s |
| mean depth pf0 / pf1 | 26.11 / 25.62 | 35.03 / 35.00 | 39.78 / 39.78 |
| positions where pf0 and pf1 choose a different move | 10 | 11 | 10 |

In these 10–14-piece positions the exemption costs little at fixed nodes (same depth at 20M/100M).

Positions where the chosen move differs (score in centipawns, side to move; ±19,9xx = database loss, ±29,9xx = forced
loss):

| # | from | pieces | pf0 1M / 20M | pf1 1M / 20M | reference 100M pf0 | reference 100M pf1 | reading |
|---|---|---|---|---|---|---|---|
| 5 | Regicide loss | 10 | 20-24 / 20-24 (−19993) | 12-16 / 12-16 (−19993) | 20-24 (−19993) | 12-16 (−19993) | already lost, equal score |
| 14 | Regicide loss | 10 | 1-5 (−19991) | 1-6 (−19991) | 1-5 | 1-6 | already lost, equal score |
| 15 | Regicide loss | 10 | 13-9 (−19993) | 8-4 (−19993) | 13-9 | 8-4 | already lost, equal score |
| 17 | Regicide loss | 10 | 24-19 (−19993) | 20-16 (−19993) | 24-19 | 20-16 | already lost, equal score |
| 24 | Regicide loss | 11 | 10-6 (−29984) | 10-7 (−29984) | 10-6 | 10-7 | already lost, equal score |
| 36 | opponent loss | 11 | 24-27 (−19987) | 31-27 (−19987) | 24-27 | 31-27 | already lost, equal score |
| 48 | opponent loss | 11 | 18-15 (−19991) | 31-27 (−19991) | 18-15 | 31-27 | already lost, equal score |
| 59 | opponent loss | 13 | 17x10 (−19992) | 24x15 (−19992) | 17x10 | 24x15 | already lost, equal score |
| 29 | Regicide loss | 11 | 24-28 (−183 / −189) | 12-16 (−189 / −198) | 24-28 (−199) | 12-16 (−201) | equal within 2; neutral |
| 52 | opponent loss | 10 | 22-26 (−187) at 1M; 12-16 at 20M | 12-16 (−184) | 12-16 (−186) | 12-16 (−187) | **pf1 finds the reference move at 1M**, pf0 by 20M |
| 11 | Regicide loss | 11 | 6-10 (−512) at 20M | 32-27 (−666) at 20M | 6-10 (−19961) | 6-10 (−19961) | lost; at 20M **pf0** matches the references, pf1 does not |
| 27 | Regicide loss | 11 | 22-26 (−19963) at 20M | 5-9 (−19959) at 20M | 22-26 (**−233**) | 5-9 (**−19963**) | the references disagree on the outcome: see below |

Reading so far: in 8 of the 12 differing positions both settings already see a loss and only pick a different losing
move (equal scores). Of the rest, one point each way (#52 for pf1, #11 for pf0), #29 is neutral, and #27 is open:
at 100M the default thinks 22-26 holds at −233, while the exemption sees a database loss. That is the kind of case
the idea is about (an opponent shot behind a forcing quiet move that the default prunes). It is being re-searched at
500M nodes with both settings.

**#27 resolved (500M nodes, both settings):** the default also sees the database loss there (22-26, −19963, depth 46;
pf1: 5-9, −19963, depth 47). The position is lost; pf1 saw it at 100M, pf0 needed 500M. So #27 counts for pf1 in
seeing the loss earlier, not in finding a better move.

## Variant prune_forcing=3 (cheap filter) and a review fix

- **Review fix (e1a73be):** the Q statistics were reset only in `Worker::setup`, so a `go` answered from the tables,
  the book or with no legal move kept the previous search's counts; they are now reset at the start of `Engine::go`.
  No posbench total above is affected: all 586 position lines were searched (none table-answered; posbench loads no
  book, and every position has ≥ 10 pieces).
- **`prune_forcing=3`:** a forcing quiet move is exempt from LMP/futility only if the forced exchange wins material
  at once. For each capture the opponent can choose after our move: (pieces our best immediate capture reply takes)
  − (pieces their capture takes); exempt only if the minimum over their choices is > 0 (`forcing_gain`, a few capture
  generations, no search). A plain one-for-one exchange stays prunable. No LMR change. (My first build also applied the
  mode-2 LMR exemption to mode 3; caught in the statistics and fixed before any measurement reported here.)
- **Cost:** bench 8,028,164 (×1.025). Middlegame set, depth 20, one E-core, measured back to back under the same load:
  mode 0 68,484,463 nodes / 173.3 s; **mode 3 63,895,559 nodes (−6.7%) / 171.6 s (−1%)**. It keeps 0.5% of the pruned
  moves (LMP 17,433 of 3,551,281; futility 119,703 of 28,311,780) against 55–65% for mode 1.

| mode | nodes to depth 20 (mid50) | time to depth 20 | 64 positions: moves differing from mode 0 at 1M / 20M / 100M | mean depth at 1M / 20M / 100M |
|---|---|---|---|---|
| 0 | 68.5M | 105.5 s (core 13) / 173.3 s (core 19, loaded) | — | 26.11 / 35.03 / 39.78 |
| 1 | 129.2M (×1.89) | 177.2 s (×1.68, core 13) | 10 / 11 / 10 | 25.62 / 35.00 / 39.78 |
| 2 | 208.4M (×3.04) | 293.0 s (×2.78, core 13) | — | — |
| 3 | 63.9M (×0.93) | 171.6 s (×0.99, core 19, loaded) | 4 / 5 / 4 | 26.00 / 34.91 / 39.62 |

Mode 3 position test (`q/out/pf3_*`), differences from mode 0:
- #36, #48 (1M/20M/100M): already lost, equal scores (as mode 1).
- #29 (1M), #30 (1M, 100M): within 3 cp of the default's move.
- #43 (20M): mode 3 plays 15-10 (−226), but both 100M references play the default's 2-6. A point against mode 3.
- **#27 (20M and 100M): mode 3 sees the database loss at 100M (5-9, −19963) like mode 1, where the default's 100M
  says −233.** The cheap filter keeps the move that matters there.

## Q screens (UHO set, 800 pairs, 2–7 database, `selfplay --threads 16`)

A = the switch on, B = defaults. 300 ms is exact time per move (master PR #37 semantics). The equal-nodes runs use
`--nodes 300000` per move for both sides, to separate the search effect from the time cost.

| test | W / D / L | draws | Elo [95% CI] | penta | pairs | ± per 800 | t | time A / B |
|---|---|---|---|---|---|---|---|---|
| Q prune_forcing=1, 300 ms (selfplay-lab6) | 604 / 359 / 637 | 22.4% | -7.2 [-12.8, -1.5] | 0 105 623 72 0 | 800 | ±5.7 | -2.50 | 262.0 / 261.2 ms, d 14.99 / 15.56 |
| Q prune_forcing=1, 300k nodes/move (selfplay-lab6) | 609 / 337 / 654 | 21.1% | -9.8 [-15.1, -4.4] | 0 103 639 58 0 | 800 | ±5.3 | -3.59 | 378.5 / 408.3 ms, d 17.72 / 18.67 |
| Q prune_forcing=3, 300k nodes/move (selfplay-lab7) | 637 / 321 / 642 | 20.1% | -1.1 [-6.3, +4.1] | 0 77 651 72 0 | 800 | ±5.2 | -0.41 | 421.3 / 400.8 ms, d 18.58 / 18.57 |
| Q prune_forcing=3, 300 ms (selfplay-lab7) | 617 / 362 / 621 | 22.6% | -0.9 [-6.4, +4.6] | 0 85 634 81 0 | 800 | ±5.5 | -0.32 | 261.5 / 261.2 ms, d 15.45 / 15.53 |

**Q outcome: no variant passes the screen bar (≥ +5, lower bound > 0), so there is no 600 ms confirmation; defaults
unchanged.**
- **Mode 1** (full exemption) is significantly worse at 300 ms (−7.2 [−12.8, −1.5]), **and also at equal nodes**
  (−9.8 [−15.1, −4.4], depth 17.72 vs 18.67). The loss is not only the time cost: spending nodes on the many forcing
  quiet moves (55–65% of the pruned ones) costs depth everywhere else.
- **Mode 3** (exempt only if the forced exchange wins material at once) is free at fixed depth and neutral in play:
  −1.1 [−6.3, +4.1] at equal nodes, −0.9 [−6.4, +4.6] at 300 ms. In the position test it finds the one real case (#27:
  a database loss the default misses at 100M nodes) as cheaply as mode 1, but that case was already lost.
- Mode 2 (LMR exemption too) was not screened: ×3.0 nodes to depth, and mode 1 already loses.
- The exemption idea does not show a measurable gain at these time controls. Mode 3 stays in the code as a cheap
  candidate for long time controls or analysis, where missing one combination matters more than depth.

# LMR sweep (valid after master PR #31: per-engine LMR table)

`selfplay-lab6` (branch with master ddd25fc merged: n8q built in, `prune_forcing` 0), **200 ms exact per move**
(with master PR #37 `--movetime` uses the full time; the old "300 ms" runs averaged ~200 ms per move, so 200 ms keeps
their depth and wall time), 800 pairs, UHO set (first 800 of the shuffled 1,600), 2–7 database, 16 games at once.
A = the value, B = defaults (lmr_base 60, lmr_mult 45). Nine candidates, so a single significant screen means
little: any value ≥ +5 with the lower bound > 0 is confirmed at 600 ms on the other half of the set (`--skip 800`)
before any default changes. Self-check (above): with the per-engine table, mean depth differs clearly between
lmr_base 20 and 120.

| test | W / D / L | draws | Elo [95% CI] | penta | pairs | ± per 800 | t | time A / B |
|---|---|---|---|---|---|---|---|---|
| LMR lmr_base=20 (200 ms) | 598 / 392 / 610 | 24.5% | -2.6 [-8.8, +3.6] | 1 109 591 99 0 | 800 | ±6.2 | -0.82 | 174.3 / 174.3 ms, d 14.35 / 14.78 |
| LMR lmr_base=40 (200 ms) | 611 / 388 / 601 | 24.2% | +2.2 [-4.1, +8.4] | 0 103 584 113 0 | 800 | ±6.2 | +0.69 | 173.7 / 174.1 ms, d 14.50 / 14.76 |
| LMR lmr_base=80 (200 ms) | 629 / 373 / 598 | 23.3% | +6.7 [+0.8, +12.7] | 0 82 605 113 0 | 800 | ±5.9 | +2.21 | 173.4 / 173.7 ms, d 14.95 / 14.76 |
| LMR lmr_base=100 (200 ms) | 602 / 397 / 601 | 24.8% | +0.2 [-5.8, +6.3] | 0 100 599 101 0 | 800 | ±6.0 | +0.06 | 174.8 / 174.7 ms, d 15.38 / 14.86 |
| LMR lmr_base=120 (200 ms) | 636 / 368 / 596 | 23.0% | +8.7 [+3.0, +14.4] | 0 70 621 108 1 | 800 | ±5.7 | +2.99 | 174.9 / 174.8 ms, d 15.54 / 14.80 |
