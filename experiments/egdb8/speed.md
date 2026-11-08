# 8-piece EGDB speed check

**Question:** in `results/baseline-30f09c3-egdb8`, Regicide's searched moves ran at a median of 2.16M nps, against
2.93M in a 7-piece control run under the same load. Does the 8-piece database slow the search, and would a cache large
enough to pre-fault all 11 GB fix it?

**Answer: no, and no fix is needed.**
- Searching the same positions, the 8-piece database is as fast as the 7-piece one, with the default on-demand mapping
  or with everything pre-faulted.
- The −26% in the baseline was a **selection effect**. With 8 pieces, the fast late-endgame positions become instant
  database moves, so the remaining searched moves are slower, fuller positions.
- At the same game phase, 7-piece and 8-piece nps are identical.
- The real slowdown against the original 7-piece baseline (about −27% at every phase) comes from the **background
  load**: gen5 datagen at 50k nodes/position instead of a 10k-node datagen.

## How the DLL sizes the EGDB cache

`src/cb_api.cpp`, master 65f7143:
- `State::egdb_cache_mb = 4096`. The first `enginecommand`/`getmove` creates the state and calls
  `load_egdb(<dll dir>/egdb, 4096)`, which calls `egdb::init(dir, cache_mb)`.
- `egdb::init` (`src/egdb/store.cpp`) loads only once (`if (!g_init_ok)`). It maps every file and pre-faults all of
  them only if the total size ≤ `cache_mb`, using `PrefetchVirtualMemory` plus a touch of every 4 KB page. Otherwise
  pages come in on demand.
- `set dbmbytes <MB>` sets `egdb_cache_mb` for a later `set dbpath <dir>`. With the default `egdb/` folder present,
  the database is already loaded by then, and a loaded database stays in use. So `set dbmbytes` has no effect on the
  standard setup.
- **Workaround, without code changes:** put the DLL in a folder **without** `egdb/`. The default load then fails and
  leaves `init` free, so `set dbmbytes 16384` followed by `set dbpath C:\...\egdb` loads with the large cache. Verified:
  `get dbpieces` → 0 before, `set dbpath` → 8 after.
- For a code change, if one is ever wanted: have `set dbmbytes` re-initialise when no search is running, or make the
  default cache larger. Given the result below, this is not needed.

## Measurement

`speed_check.py` drives `cbhost.exe` with the 65f7143 DLL (n4c, no book; `engines/regicide/65f7143-noegdb`, no
`egdb/`):
- **Positions:** 88 from the 8-piece baseline games (`speed_fens.py` → `speed_fens.txt`): 7–8 pieces 16, 9–10 pieces
  32, 11–12 pieces 24, 16–20 pieces 16.
- **Searches:** `go maxtime 0.5 reset`, threads 1, hash 128.
- **Speed measure:** nps = total nodes / total `getmove` time, over moves that searched ≥ 0.2 s. Forced captures and
  instant database moves are excluded.
- **Concurrency:** 1 process, and 8 at once (like `-c 8` games).
- **Machine:** all background generators stopped (state `T`), 18:44–18:47. Windows RAM: 63.7 GB, 44.9 GB free before
  and 45.3 GB free after.

| Setup | Procs | Load time | Working set / proc | Private / proc | nps 9–10 pc | nps 11–12 pc | nps 16–20 pc | nps all |
|---|---|---|---|---|---|---|---|---|
| 7-piece (1 GB, pre-faulted at 4096 MB) | 8 | 0.24 s | 1.11 GB | 0.13 GB | 2.73M | 2.49M | 2.29M | **2.70M** |
| 8-piece, 4096 MB (on demand) | 8 | 0.01 s | 0.14 → 1.02 GB | 0.15 GB | 3.34M | 2.53M | 2.50M | **2.76M** |
| 8-piece, 16384 MB (pre-faulted) | 8 | 2.6 s (max 3.2) | 10.15 GB | 0.15 GB | 3.09M | 2.67M | 2.51M | **2.73M** |
| 7-piece | 1 | 0.27 s | 1.11 GB | 0.13 GB | 1.83M | 1.54M | 1.36M | 2.00M |
| 8-piece, 4096 MB | 1 | 0.01 s | 0.14 → 0.85 GB | 0.15 GB | 3.66M | 1.57M | 1.56M | 2.04M |
| 8-piece, 16384 MB | 1 | 2.8 s | 10.15 GB | 0.15 GB | 1.96M | 1.69M | 1.48M | 1.68M |

- **Speed:** with 8 processes, the three setups are within 2% of each other overall. At 9–10 pieces, 8-piece is
  *faster*, because 8-piece positions inside the search are cut off by the database.
- **Single process:** these runs are noisy, with only 16–27 searched moves each. The 285K mixes P- and E-cores, so one
  process can land on either. They show no 8-vs-7 penalty either.
- **Memory:** the pre-faulted 11 GB is shared file cache. Each process's working set counts it (10.15 GB), but private
  commit stays at 0.15 GB, and Windows free RAM did not change. Pre-faulting costs about 3 s per engine process at
  startup and gains nothing in speed.

## The baseline's nps, by game phase

Median nps of Regicide's searched moves (≥ 0.3 s), by Regicide move index (`npsply.py`), with move counts in brackets:

| Run (background) | moves 0–4 | 5–9 | 10–14 | 15–19 | 20–24 | 25–29 | 30–34 | 35–39 | 40+ |
|---|---|---|---|---|---|---|---|---|---|
| KingsRow 7-piece control (gen5) | 2138 [665] | 2073 | 2159 | 2299 | 2544 | 2746 | 2884 | 3021 | 3580 [6235] |
| KingsRow 8-piece (gen5) | 2130 [664] | 2081 | 2181 | 2224 | 2256 | 2302 | 2363 | 2398 | 2628 [110] |
| KingsRow 7-piece eb1037c (gen4, 10k nodes) | 2910 [692] | 2835 | 2928 | 3095 | 3359 | 3645 | 3825 | 3906 | 4137 [5942] |
| Cake 7-piece eb1037c (gen4) | 2909 | 2834 | 2915 | 3132 | 3375 | 3638 | 3825 | 3920 | 4156 |
| Cake 8-piece (gen5) | 2183 | 2129 | 2256 | 2342 | 2329 | 2442 | 2646 | 2855 | 3048 [184] |

- **Up to move 15, 7-piece and 8-piece under the same load are identical** (2138 vs 2130, 2073 vs 2081, 2159 vs 2181
  kN/s).
- **Later, the 7-piece run has thousands of fast endgame searches** (6,235 at move 40+). The 8-piece run plays those
  instantly from the database (only 110 searched), which pulls the 7-piece median up.
- **Against the original 7-piece baseline, every phase is about 27% slower under gen5.** This is the background: gen5
  searches 50k nodes/position on 12 threads, and its larger per-thread hash tables likely press harder on the shared
  cache and memory. The earlier datagens searched 10k nodes.

## Re-check with master 769ef0b (cache fix)

769ef0b (PR #20) changes two things:
- `egdb::init` pre-faults files smallest first, within the budget. Before, pre-faulting was all-or-nothing.
- `set dbmbytes N` now raises the budget of the already-loaded database.

`speed_check2.py` (log `speed/speed_check2.txt`) uses the engine dirs as they are used in matches:
- `769ef0b-egdb7` (2–7) and `769ef0b-egdb8` (2–8), each with its own `egdb/` folder;
- `set dbmbytes 12288` sent after the first load for the third setup;
- the same 88 positions, two rounds, generators stopped (19:03–19:09).

| Setup | Working set / proc | `set dbmbytes` time | nps 9–10 pc (rounds 1, 2) | 11–12 pc | 16–20 pc |
|---|---|---|---|---|---|
| 7-piece (default) | 1.11 GB | — | 2.77M, 2.96M | 2.62M, 2.67M | 2.40M, 2.39M |
| 8-piece, default 4096 MB (now 4.09 GB pre-faulted) | 4.09 → 4.40 GB | — | 3.00M, 3.03M | 2.48M, 2.59M | 2.35M, 2.45M |
| 8-piece + `set dbmbytes 12288` (all pre-faulted) | 10.15 GB | 1.6–2.5 s | 2.87M, 2.97M | 2.50M, 2.52M | 2.37M, 2.35M |

All rows use 8 concurrent processes. Private memory is 0.13–0.15 GB per process in every setup.
- The fix works as described: the default now pre-faults 4.09 GB, and `set dbmbytes 12288` raises it to the full
  11 GB in about 2 s.
- **The speed is the same in all three setups**, within run-to-run noise (±4%). Over all searched moves, the 8-piece
  default gave 2.63M and `set dbmbytes 12288` 2.58M; the two-round means differ by under 2%.
- **Setting chosen for the book/avoid tests: the default** (no `set dbmbytes`), using the 769ef0b DLL with the 2–8
  folder. `switch769.sh` chooses `set dbmbytes 12288` only if it is more than 2% faster.

## Consequences

- **The 8-piece baseline result is valid as an 8-piece result.** It ran at full 8-piece speed. Against KingsRow, 7 vs
  8 pieces under the same load is −0.0 vs −0.0.
- **The book/avoid tests (A and B) use the default 8-piece DLL setup** (on-demand, 4096 MB). The large cache gains
  nothing.
- **Background load matters for absolute strength:** about −27% nps from a 50k-node datagen. Runs compared with each
  other should share the same background. The book/avoid tests do: one ~10–12-thread generator throughout, bookgen
  until the book is done, then gen5.
