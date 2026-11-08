# Baseline with the 8-piece EGDB: Regicide 30f09c3 (n4c) vs KingsRow, Cake, GuiNN

## Engine and setup

- **Engine:** the same `regicide_cb.dll` as `results/baseline-eb1037c-n4c`: master 30f09c3, NNUE n4c (FT 1024), no
  book. Its `egdb/` folder is a full copy of the 2–8 piece database (`experiments/egdb8`). Through `cbhost.exe` it
  reports `dbpieces 8`. It maps the 11 GB on demand, because the DLL's fixed 4,096 MB cache is too small to pre-fault
  it.
- **Settings:** exactly as the 7-piece baseline:
  - `acf156` (156 ballots × 2 = 312 games), `--movetime 0.5`, `-c 8`;
  - Regicide hash 128, threads 1;
  - opponents with their own books and the KingsRow 2–8 WLD database (`dbmbytes 512`), hash 128; KingsRow
    searchthreads 1.
- **Scripts:** `run_baseline_r4.sh` (`NAME=30f09c3-egdb8`), via `baseline_egdb8.sh`.
- **Machine load:**
  - gen5a datagen (master 635d949, **50k nodes**/position, 12 threads) ran alongside. The 7-piece baseline had a 10k-node
    datagen (gen4a/gen4b) instead.
  - Bookgen and gen4b were paused. No other matches or training ran.
- **Run times:** KingsRow 16:51–17:05, Cake 17:05–17:18, GuiNN 17:18–17:32.

## Results

| Opponent | W / D / L | Elo [95% CI] (pentanomial) | Pentanomial | 7-piece (eb1037c-n4c, same DLL) |
|---|---|---|---|---|
| KingsRow 1.20 | 1 / 310 / 1 | **−0.0** [−3.1, +3.1] | [0 1 154 1 0] | −4.5 [−8.8, −0.1] (0/308/4) |
| Cake 1.89f | 0 / 291 / 21 | **−23.4** [−32.8, −14.1] | [0 21 135 0 0] | −15.6 [−23.4, −7.8] (0/298/14) |
| GuiNN 2.06 | 2 / 307 / 3 | **−1.1** [−6.0, +3.8] | [0 3 151 2 0] | −3.3 [−9.9, +3.2] (3/303/6) |

**Decisive games:**
- KingsRow: lost Black Widow (W), won ACF #37 (W).
- Cake: 21 losses, 18 as Black.
- GuiNN: lost Edinburgh Single (B), Double Cross (W) and White Doctor (B); won ACF #111 (B) and ACF #128 (B).

**Lost games were checked against the database.** The Cake loss inspected in detail (Wilderness II) went wrong in the
opening: Regicide's own score fell from −42 to −234 by move 20, and it reported a forced loss (−19973) at move 22, long
before 8 pieces. Nothing points to a wrong database value.

## Two things changed besides the database

| KingsRow run | Background | Regicide median nps (moves ≥ 0.3 s) | Median depth | Instant moves (< 0.05 s) | Result | Run time |
|---|---|---|---|---|---|---|
| 7-piece (eb1037c-n4c baseline) | 10k-node datagen | 3.74M | 25 | 44% | −4.5 | 25 min |
| **7-piece control (`kingsrow-7piece-control/`)** | 50k-node gen5 | **2.93M** | 24 | 43% | **−0.0 [−4.4, +4.4]** (2/308/2) | 25 min |
| 8-piece | 50k-node gen5 | 2.16M | 21 | 77% | −0.0 [−3.1, +3.1] | 14 min |

1. **Speed.** On searched moves, Regicide ran about 40% slower than in the 7-piece baseline. The drop is constant from
   the first block of 52 games to the last, so it is not a cold page cache. The control run (the 7-piece DLL vs
   KingsRow under the 8-piece run's load, run afterwards) splits it:
   - about **−22% comes from the heavier background** (gen5 at 50k nodes instead of a 10k-node datagen);
   - about **−26% comes from the 8-piece database** in the Windows DLL (2.93M → 2.16M, same load, same DLL).

   On Linux the same 2–7 vs 2–8 comparison on 120 game positions cost only about 3% median nps. Windows
   `regicide.exe` tests ranged from −4% to −22%, depending on load. So the cost seems specific to on-demand mapping
   on Windows, where the 7-piece set (1 GB) was pre-faulted. **Suggestion:** a DLL default cache large enough to
   pre-fault 11 GB (or `dbmbytes` taking effect before the first load) should be tested.
2. **Instant moves.** With 8 pieces, 60–77% of Regicide's moves are instant root-database moves, against 27–44% before.
   Games are shorter (Cake: 12,971 Regicide moves instead of 19,342), and the runs took about 14 min instead of 25.
   Against Cake and GuiNN almost every draw now ends by repetition (291 and 305), where before many ended by the
   no-progress rule.

**Reading:**
- **KingsRow:** under the same load, 7 and 8 pieces give the same result (−0.0 vs −0.0). The earlier −4.5 with 7 pieces
  is within noise of both.
- **Cake** (−23.4 vs −15.6) and **GuiNN** (−1.1 vs −3.3): the CIs overlap, and these runs have no 7-piece control under
  the same load.
- The 8-piece database does not show a measurable gain at 0.5 s/move. Its Windows speed cost (−26%) would have to be
  fixed before it can.

## Files

- `kingsrow/`, `cake/`, `guinn/`: `summary.txt`, `results.csv`, `losses.pdn`, `runner.log`.
- `kingsrow-7piece-control/`: the control run (the 7-piece DLL `engines/regicide/30f09c3`, same settings and load).
- `nps_analysis.txt`: per-run nps, depth and instant-move statistics from the PDN comments, plus the Windows and
  Linux 2–7 vs 2–8 speed tests (`nps.py`, `nps2.py`, `probe78.py`, `probe78win.py`, `nps_check.txt`).
- `BUILD.txt`, `run_baseline_r4.sh`, `baseline_egdb8.sh`, `baseline_7ctl.sh`, `timeline.txt`.
