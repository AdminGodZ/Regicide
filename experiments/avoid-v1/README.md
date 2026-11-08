# avoid-v1: avoid list from 103 lost games against KingsRow, Cake and GuiNN

`avoid_v1.bin` is a version-2 book file with **79 avoid entries and no book moves**. lossprobe produced it at master
635d949 from every lost game in the three newest baselines. `report_v1.txt` is the full lossprobe report: per game,
every analysed Regicide position (played move and score, best/alternative move and score, depths, PV) and the blame.

## Setup

- Laptop: i9-13900H (20 threads), Arch Linux, clang 23.1.4 Release, on AC, sleep inhibited, nice 10.
- Build 635d949: ctest 13/13, `regicide bench` = 7,338,658 nodes.
- **EGDB**: 2-7 pieces built on the laptop with `egdb_gen build --max 7 --mtc 6 --threads 18 --dir ~/regicide-data/egdb`.
  It took 19 min (1,145 s; 7 pieces alone 1,018 s) and is 1,002 MB. `egdb_gen manifest` exited 0 with all 414 files ok
  (`egdb_manifest_laptop.txt`). No 8-piece tables.
- lossprobe command (Regicide's side from the PDN headers, ballot 3 plies, defaults drop 30 / margin 20 / lost 300):

```
lossprobe --pdn results/baseline-eb1037c-n4c/{kingsrow,kingsrow-2s,cake,guinn}/losses.pdn \
  --pdn results/baseline-ae210fb/{kingsrow,cake,guinn}/losses.pdn \
  --pdn results/baseline-747ab51/{kingsrow,cake,guinn}/losses.pdn \
  --movetime 10000 --threads 6 --jobs 3 --hash 1024 --egdb ~/regicide-data/egdb \
  --out avoid_v1.bin --report report_v1.txt
```

- Run time **1 h 15 min** (15:55 → 17:11, 2026-11-08), about 8.2 searched positions per minute (two 10 s searches each,
  3 jobs). Games stop at the first blame, so only 725 positions were searched (plus 192 forced moves skipped), not
  the ~1,700 a full walk would take. Package temperature stayed around 63 °C.

## Counts

| | |
|---|---|
| games (lost) | **103**: Cake 59, GuiNN 33, KingsRow 9, KingsRow-2s 2 |
| blamed | **92** (Cake 51, GuiNN 31, KingsRow 8, KingsRow-2s 2) |
| unclear (no move meets the blame rule) | **11** |
| errors | 0 |
| avoid entries (distinct position + move) | **79** |
| blames within the first 10 moves after the ballot (ply − ballot < 20) | **71 / 92** (39 within 5 moves) |
| entries within the first 10 moves after the ballot | **59 / 79** (29 within 5 moves) |
| drop (played vs previous score), cp | min 30, median 62; 3 drops are into database losses (`dbloss/25-28`) |

Blame plies after the ballot: 0-9: 39, 10-19: 32, 20-29: 10, 30-39: 7, 40-49: 1, 60+: 3. Most losses come from an
early opening/middlegame mistake, so the list mostly acts like an opening-book correction.

Entries blamed in more than one game (one entry each, `count` = games). These are mistakes the engine repeats across
baselines and against different opponents:

| move | position | games |
|---|---|---|
| 9-14 | `B:W16,19,20,21,22,26,27,28,29,30,32:B1,2,3,4,5,7,9,10,11,12` | 5 (ae210fb Cake ×2, GuiNN; eb1037c Cake, GuiNN) |
| 29-25 | `W:W17,21,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,10,11,12,13,15` | 4 (747ab51 Cake, KingsRow; ae210fb Cake; eb1037c Cake) |
| 24-28 | `B:W10,15,18,29,31,32:B2,5,8,12,22,24` | 2 (eb1037c Cake, GuiNN) |
| 7-10 | `B:W19,21,22,23,25,27,28,29,30,31,32:B2,3,4,5,6,7,8,11,12,13,14` | 2 (747ab51 Cake, ae210fb Cake) |
| 6-10 | `B:W14,19,23,25,26,27,28,31,32:B3,4,5,6,7,8,12,13,20` | 2 (ae210fb Cake) |
| 16-20 | `B:W19,21,22,23,24,25,27,29,30,31,32:B1,2,4,6,7,8,10,11,12,14,16` | 2 (ae210fb Cake, GuiNN) |
| 2-7 | `B:W13,21,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,8,9,10,11,12,14` | 2 (747ab51 GuiNN, ae210fb GuiNN) |
| 14x23 | `B:W7,18,20,21,22,26,27,30,31,32:B1,2,4,6,8,9,12,13,14` | 2 (747ab51 Cake, GuiNN) |

Unclear games: eb1037c Cake #1, #3, #4, #14 and GuiNN #2; ae210fb KingsRow #2, Cake #8, #11, #20 and GuiNN #3;
747ab51 Cake #20. In these, no single move dropped ≥ 30 with a ≥ 20 better alternative before the position was lost.
They are long games (108-262 plies), so these were probably slow endgame losses.

## Sanity check: 5 random blamed entries

Sampled with `random.seed(1)` from the 92 blames. Each played move and alternative was re-searched independently in
the CLI (`position fen … moves <m>`, then `go`, score negated). The first pass used 20 s at 2 threads with the EGDB.
Then the avoid list was loaded (`set avoidfile avoid_v1.bin`) and a 2 s root search was run on each position.

1. **eb1037c GuiNN #3**, ply 12 (5th move after the ballot), Black, `B:W16,19,20,21,22,26,27,28,29,30,32:B1,2,3,4,5,7,9,10,11,12`.
   lossprobe: 9-14 −71 vs 11-15 −19 (previous −17, drop 54). Re-search: 9-14 **−76**, 11-15 **−22**. **Confirmed.**
   This is also the most-repeated entry (5 games).
2. **747ab51 Cake #21**, ply 16, Black, `B:W14,18,21,23,24,25,26,27,28,30,31:B2,3,4,5,6,7,9,11,12,17,20`.
   lossprobe: 12-16 −136 vs 11-15 −23 (drop 100). Re-search: 12-16 **−134**, 11-15 (after the forced 18x11) **−25**.
   **Confirmed.** The first try at 11-15 returned at depth 1 because White's reply is forced.
3. **eb1037c Cake #6**, ply 37 (18th move), White, 6 v 6 endgame `W:W12,16,17,24,29,31:B3,7,8,10,15,18`.
   lossprobe: 17-13 −200 vs 24-20 −113 (previous −39, drop 161). Re-search at 20 s / 2 threads: 17-13 −130,
   24-20 −152. At 30 s / 6 threads: 17-13 **−125**, 24-20 **−109**. **Doubtful.** The alternative is better by only
   16 cp, below the margin of 20, and the two lines transpose (each PV plays the other move next). Both moves are
   well below the previous −39, so the real mistake was probably earlier. The entry is harmless: it only blocks 17-13
   in this exact position.
4. **ae210fb Cake #13**, ply 12: the same position and move as #1 (9-14), from another game. Re-search: 9-14 **−71**,
   11-15 **−20**. **Confirmed.**
5. **eb1037c Cake #13**, ply 19 (9th move), White, `W:W18,19,22,23,24,26,28,30,32:B1,2,3,4,6,7,9,12,21`.
   lossprobe: 24-20 −124 vs 32-27 −13 (previous −6, drop 118). At 20 s / 2 threads (depth 24) both moves scored the
   same: 24-20 −7, 32-27 −6. At 30 s / 6 threads (depth 26): 24-20 **−128**, 32-27 **−4**. **Confirmed**, but the
   refutation is deep, past depth 24.

With the avoid list loaded, the root search avoided the blamed move in all 5 positions. It chose the lossprobe
alternative each time (11-15, 11-15, 24-20, 11-15, 32-27).

**Verdict:** 4 of 5 are confirmed with a large, reproducible drop (50-120 cp), and 1 is a borderline endgame entry.
The 10 s blames look trustworthy in the opening and early middlegame, where 59 of the 79 entries are. Later endgame
entries are noisier. A second pass that re-checks the 20 entries from the 10th move after the ballot onward at 30 s,
before embedding, would remove the doubtful ones.

## Files

- `avoid_v1.bin`: the avoid list (79 entries), sha256 `a81be6bba2571f0a438d1d534c50ea153889f5b9e6ba0a821444124f19c4fa2c`
- `report_v1.txt`: the full lossprobe report
- `egdb_manifest_laptop.txt`: manifest of the laptop's 2-7 piece EGDB (checksums, for comparing with other machines)
