# Windows/WSL compute box: state on 2026-11-09 (before a pause)

Machine: Win11-AdminGod (Intel Core Ultra 9 285K, 24 cores, 64 GB RAM, RTX 5080), WSL2 Ubuntu 24.04 (user `admingod`).
Everything below lives **only on this machine** unless a branch is named. Paths starting with `~` are inside WSL
(`/home/admingod`). `C:\regicide-match` is `/mnt/c/regicide-match`.

## Data that exists only here

### Training data (`~/regicide-data/train`, 5.5 GB; 16-byte DataRecords)

| File | Bytes | Records | Notes |
|---|---|---|---|
| gen1_n10k.bin | 154,288,128 | 9.6M | 10k nodes |
| gen1b_n10k.bin, gen1c_n10k.bin | 477,080,736 / 477,064,160 | 29.8M each | 10k nodes |
| gen2a_n10k.bin, gen2b_n10k.bin | 480,975,952 / 472,261,552 | 30.1M / 29.5M | 10k nodes |
| gen3a, gen3b, gen3c, gen3d _n10k.bin | 480.5M / 470.4M / 458.9M / 457.6M | 28.6–30.0M each | 10k nodes |
| gen4a_n10k.bin | 449,331,728 | 28,083,233 | 10k nodes, n4c engine (eb1037c), seed 53 |
| **gen4b_n10k.bin** | 87,744,512 | 5.48M | **unfinished**: 10k nodes, n4c, seed 54; process paused (SIGSTOP, `gen4bhold`) at about 53k of 300k games |
| **gen5a_n50k.bin** | 166,961,920 | 10,435,120 | **50k nodes**, master 635d949, seed 61, 109,330 games |
| **gen5c_n50k.bin** | 166,389,120 | 10,399,320 | **50k nodes**, master 635d949, seed 62, 109,130 games; **finished 2026-11-09 12:49** |
| `*_partial_*.bin`, gen1b_snap.bin | | | whole-record prefix snapshots used by single runs (duplicates of the files above; not to be trained on together with the full file) |

gen5b (50k nodes, seed 71) is on the laptop, not here.

### Endgame database

- **`~/regicide-data/egdb`:** 2–8 pieces, 11 GB, 561 files, plus an empty `parts/`. The 8-piece WLD set was built here
  on 2026-11-08 (`experiments/egdb8` on `t3/egdb8`); the database files are not in git.
- **Windows copies:** each of these engine dirs has a full 11 GB copy in `egdb/` (several could be deleted to save
  space):
  - 30f09c3-egdb8, 65f7143-egdb8, 769ef0b-egdb8, 769ef0b-bookavoid-egdb8;
  - 9841716-egdb8, b88c02e-egdb8, ff0a3d2-deep-egdb8, 6bb8c6c-egdb8;
  - 6bb8c6c-deep2-egdb8 (after today's build).
- **2–7 copies (1 GB):** the older dirs.

### Opening books (`~/regicide-data/book`)

| File | Bytes | On a branch? |
|---|---|---|
| book_deep.bin | 520,668 | yes: master `src/weights/book.bin` (6bb8c6c), `t3/book-deep` |
| **book2.bin** | (growing) | pass 2: 5 roots at 150M nodes/move. Pushed to `t3/book-deep2` if finished in time |
| **book_deep2.bin**, book_deep2_merged.bin | | book_deep + book2 + 6 avoid entries; pushed to `t3/book-deep2` if finished in time |
| book_n4c.bin, book_n4c_avoid.bin | 2,970,508 / 2,966,064 | yes, `t3/book-n4c` |
| avoid_v1.bin | 1,280 | yes, `t3/avoid-v1` |
| book_deep_acf156.bin | 230,492 | yes, `t3/book-deep-acf156` (Arch) |
| book_b13.bin | 3,411,804 | older linear-eval book from earlier work (not checked against a branch) |
| `*.calib` | | calibration snapshots of the deep books |

### Nets and checkpoints

- **Published `.nnue` files:**
  - `~/regicide-nnue4-work` (n4a–n4d): `t3/nnue4`;
  - `~/regicide-nnue5-work` (v2a–v2g, l5a–l5c): `t3/nnue5`;
  - `~/regicide-nnue6-work` (n6a–n6f): `t3/nnue6`;
  - `~/regicide-nnue7-work` (n7a, n7b): `t3/book-deep2`, `experiments/nnue7`.
- **Checkpoints, only here** (`runs/<net>/ckpt_e*.pt`, `last.pt`, per-epoch nets):
  - nnue4 373 MB, nnue5 1.6 GB, nnue6 487 MB, nnue7 224 MB.
  - Needed to fine-tune or `--resume` any of them, e.g. `~/regicide-nnue4-work/runs/n4c/last.pt`, the base of n6a.
- **Validation sets used for scoring:**
  - `~/regicide-nnue5-work/val2/nnue_valid_1M.bin` (shared set);
  - `~/regicide-nnue6-work/val/gen5a/nnue_valid_1M.bin` (gen5a's held-out tail);
  - configs in `~/regicide-nnue6-work/val/*.json`.

### Engines and builds

- **CheckerBoard DLLs:** `C:\regicide-match\engines\regicide\<sha>[-variant]\regicide_cb.dll`, each with a `BUILD.txt`.
- **Build trees:** `~/regicide-match/builds/<name>/{src,build,build-linux}`, git worktrees of `~/regicide-nnue`.
  35 worktrees in all, e.g. `~/regicide-385`, `~/regicide-v2`, `~/regicide-ff0`, `~/regicide-pub*` (publishing).
- **Match runner and cbhost.exe:** `~/regicide-match/bin/match-747ab51`, `C:\regicide-match\bin\b71dfa9\cbhost.exe`.
- **Opponents:** KingsRow / Cake / GuiNN in `C:\regicide-match\engines\{kingsrow,cake,guinn}`, KingsRow 8-piece WLD
  database `C:\regicide-match\db\kingsrow_wld8`.

### Jobs, logs, raw results

- **Job scripts:** `C:\regicide-match\jobs\{r3,r4,r5,r6,r7,egdb8,ba,book,cake,tm,deep,deep2,deep3}`.
- **Status and logs:** `~/regicide-jobs/<job>/`. The master status log is `~/regicide-jobs/r4/status`.
- **Raw match results**, with every game PDN: `~/regicide-match/results/{baseline-*,ba-*}`, `~/regicide-jobs/tm/matches`,
  `~/regicide-nnue*-work/matches`. Only summaries, `results.csv` and losses were pushed.

## Processes and locks at the pause

- **Keepalives:** `keepalive_r4.sh` holds the VM while `~/regicide-jobs/r4/hold` exists (and the old r3 one while
  `~/regicide-jobs/r3/hold` exists). To let WSL stop, remove both hold files.
- **`pausekeeper5.sh`** (`~/regicide-jobs/r4`) stops and resumes background generators through lock files in
  `~/regicide-jobs/r4`:
  - `matchlock`: everything stopped;
  - `baselock`: bookgen stopped;
  - `gen4bhold`: gen4b stopped;
  - `gen5yield`: gen5 yields to bookgen.
- **gen4b datagen** (process: `pgrep -f datagen-eb1037c`) is **stopped** and held by `gen4bhold`. Resume it by deleting
  `gen4bhold`; it is not needed.
- gen5c has finished. No other job is running after the deep2 tests.

## Pending jobs and how to resume them

All need an attached `wsl.exe` session if they play CheckerBoard engines (Windows interop):
`Start-Process -WindowStyle Hidden wsl.exe -ArgumentList 'bash','<script>'`.

1. **deep2, full runs** (skipped for the 22:00 deadline): book_deep2 vs Cake (acf156, 312 games) and vs KingsRow, same
   settings.
   `bash /mnt/c/regicide-match/jobs/deep2/run_ba3.sh 6bb8c6c-deep2-egdb8 deep2-full cake 0.5`, then the same with
   `kingsrow`. Compare with the book_deep on-runs (`ba-deep-on{,2,3}-cake`, `ba-deep-on-kingsrow`) using
   `python3 /mnt/c/regicide-match/jobs/tm/pool.py "deep2=ba-deep2-full-cake" "deep=ba-deep-on-cake,ba-deep-on2-cake,ba-deep-on3-cake"`.
2. **Round-7 matches vs n6c** (nets trained and scored; not matched): `bash /mnt/c/regicide-match/jobs/r7/r7_match.sh`.
   - It waits for `D2 D2_ALL_DONE` and `R7 R7_SCORED` in `~/regicide-jobs/r4/status`. Append
     `D2 D2_ALL_DONE (manual)` to that file first, or remove the wait line.
   - It plays n6d, n7a, n7b vs n6c on the ff0a3d2 `.so` (0.1 s/move, 1,610 games each, datagen paused). About 35 min.
3. **book_deep3** = base (book_deep2 if it passes, else book_deep) + Arch's acf156 book:
   `/mnt/c/regicide-match/jobs/deep3/d3_chain.sh`.
   - It waits for `R7 R7_DONE`; remove that line to run it directly.
   - It also needs `ba-deep2-full-*` results for its base decision. Otherwise set `BASE=` by hand.
   - It merges, re-saves, builds DLL `6bb8c6c-deep3-egdb8`, and runs a Cake pair (plus a second pair if promising) and
     KingsRow.
4. **NNUE round 6c** with all deep data (gen5a + gen5b from the laptop + gen5c here): copy gen5b to
   `~/regicide-data/train/`. Then reuse `C:\regicide-match\jobs\r6\r6b_train.sh` (the n6f line) with the three full
   50k files instead of the prefix.
5. **gen4b**: optional; delete `gen4bhold` to resume it (10k-node data, low value now).

Branches pushed from this machine (all non-master): t3/nnue4, t3/baseline-eb1037c-n4c, t3/nnue5, t3/egdb8,
t3/book-n4c, t3/book-avoid-test, t3/cake-analysis, t3/tm-tune, t3/nnue6, t3/book-deep, t3/book-deep2.
