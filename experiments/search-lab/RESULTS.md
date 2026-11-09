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

(calibration matches pending)
