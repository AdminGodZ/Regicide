# gen5b: 50k-node self-play training data (laptop, seeds 71 + 72)

This complements gen5a (Windows: seed 61, 50k nodes, 120k games), using the same engine and settings. **The data
file stays local** (too big for git):

| | |
|---|---|
| file | `~/regicide-data/train/gen5b_n50k.bin` (on the Arch laptop) |
| size | **132,702,544 bytes** (126.6 MiB) = 8,293,909 records × 16 bytes, which matches datagen's counts exactly |
| sha256 | `c0dd2900ac16b52bc7d516b85f978b3a057df16f19f5035bfe44f979ce68513c` |

## Engine and command

- master **769ef0b**, clang 23.1.4 Release, `-march=native` on an i9-13900H. ctest 13/13, `regicide bench` = 7,338,658.
  It plays like the gen5a engine: built-in n4c NNUE. The 2-7 EGDB exists on the laptop but is **not** passed to
  datagen, so it is not used, same as gen5a.
- Default random openings (`--random-min`/`--random-max` and the opening-score filter left at their defaults, as in
  gen4/gen5a).
- Two runs appending to the same file, each a transient systemd user service, nice 10, sleep inhibited:

```
datagen --nodes 50000 --games 80000 --threads 18 --seed 71 --out ~/regicide-data/train/gen5b_n50k.bin
datagen --nodes 50000 --games 15000 --threads 18 --seed 72 --out ~/regicide-data/train/gen5b_n50k.bin   # top-up
```

The rate after 15 minutes put the 80k run at ~9 h, below the 10-12 h target. Restarting with the same seed into the
same file would have duplicated games (each thread's games are fixed by the seed), so a 15k-game top-up with a
different seed was chained to start when the first run ended cleanly.

## Counts and timing

| run | start | end | wall time | games (completed) | positions | pos/game |
|---|---|---|---|---|---|---|
| seed 71, `--games 80000` | 2026-11-08 19:13:13 | 2026-11-09 03:19:37 | 8 h 06 m | 72,983 | 6,971,518 | 95.5 |
| seed 72, `--games 15000` | 2026-11-09 03:19:46 | 04:52:04 | 1 h 32 m | 13,739 | 1,322,391 | 96.2 |
| **total** | | | **9 h 39 m** | **86,722** | **8,293,909** | 95.6 |

- `--games` counts attempts. About 8.7% of the random openings were skipped by the opening-score filter (and are not
  written), so 95,000 attempts gave 86,722 games.
- Throughput: about 150 completed games/min, ~14,300 positions/min.
- Results (Black's view) at the last progress line of each run: seed 71 at 72,000 games: +18,761 =33,281 −19,958;
  seed 72 at 13,000 games: +3,323 =6,068 −3,609. That is ~46% draws, with White slightly ahead, as expected after
  random openings.
- Thermals (`monitor.log`, 576 one-minute samples): package mean 63.2 °C, max 65 °C, apart from the two ~100 °C
  samples taken during the few-second boost at each run's start. Mean clocks were 1.6 GHz P-core / 1.15 GHz E-core
  (power-limited by the `balanced` firmware profile), and the laptop was on AC throughout.
