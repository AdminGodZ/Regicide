# egdb8: the 8-piece WLD endgame database (built 2026-11-08, Win11-AdminGod / WSL2)

**Result:**
- **Build:** all 8-piece WLD slices, **147 files, 9.70 GB, 447.2 G index entries**, in **2 h 17 min** at 8 threads.
- **Checks:**
  - `egdb_gen verify` found 0 mismatches over 50,000 samples in every new slice.
  - `egdb_gen manifest` reports all 561 files ok.
  - `egdb_test --data` passes with `max_pieces 8`.
- **CheckerBoard DLL:** with the 2–8 piece copy, the DLL reports `dbpieces 8`.
- **Not committed:** the database files. They stay in `~/regicide-data/egdb` on that machine.

The 8-piece CheckerBoard baseline, with the same engine as the 7-piece baseline, is in
`results/baseline-30f09c3-egdb8` (added when it finishes).

## Build

- **Tools:** `egdb_gen` and `egdb_test` from master `35ae5ea`, Release, clang 22, `-march=native` (`scripts/prereq.sh`).
  `src/egdb` and `tools/egdb_gen.cpp` are unchanged since 3c87728, whose `src/egdb/README.md` describes the
  8-piece build.
- **Prerequisites** (`logs/manifest_before.txt`, `logs/egdb_test_data_before.txt`):
  - the complete 2–7 piece set in `~/regicide-data/egdb`;
  - `egdb_gen manifest` exited 0;
  - `egdb_test --data` passed;
  - 903 GB free on the WSL disk and 25 GB RAM available.
- **Command chain** (`scripts/egdb8_chain.sh`), run detached under the job keepalive and resumable:

```
egdb_gen build --max 8 --threads 8 --sides 44 --dir ~/regicide-data/egdb   # 4v4
egdb_gen build --max 8 --threads 8 --sides 53 --dir ~/regicide-data/egdb   # 5v3
egdb_gen build --max 8 --threads 8            --dir ~/regicide-data/egdb   # the rest: 6v2, 7v1
```

| Step | Families | Wall time | CPU | Max RSS | 8-piece files | Size |
|---|---|---|---|---|---|---|
| `--sides 44` | 4v4 | 40:46 | 775% | 9.3 GB | 25 | 8.31 GB |
| `--sides 53` | 5v3 | 1:02:03 | 791% | 3.5 GB | 48 | 1.20 GB |
| rest | 6v2, 7v1 | 33:55 | 779% | 3.1 GB | 42 + 32 | 0.18 + 0.01 GB |
| **total** | | **2:16:44** | | | **147** | **9.70 GB** |

Comparison with the README estimate (~2.2 h nominal, 3–4.5 h allowing for larger tables, ~5 GB RAM, 5–9 GB on
disk):
- The time came in at the nominal end. 4v4 took 41 min (estimated ~1 h) and 5v3 62 min (estimated 1.5–2 h).
- **RAM peaked at 9.3 GB in 4v4**, about twice the estimate. The machine has 31 GB, so it was harmless. `time -v`
  counts the resident mapped pages of the 7-piece files and of the slices finished during the run, so the working set
  proper is smaller.
- **Disk: 9.70 GB, slightly above the 5–9 GB estimate.** 4v4 compresses worst: 8.31 GB for 28% of the entries.
- The whole database (2–8 pieces, WLD up to 8, MTC up to 6) is now 11 GB in 561 files.

**Machine load during the build:**
- 14:27–15:12: the NNUE round-5 matches (`-c 10`) and the round-5b GPU training (about 1.2 cores).
- From 15:15: bookgen at 10 threads, `nice 5`, for the NNUE book; it was paused during A/B matches.
- gen4b datagen was paused for the whole build.

## Validation

- **`egdb_gen verify --pieces 8 --samples 50000 --threads 8`** (`logs/verify8.txt`):
  - Every one of the 147 slices was sampled 50,000 times, as checked positions plus capture positions.
  - TOTAL: 3,613,877 positions checked with the one-ply consistency rule; **0 WLD mismatches**.
  - It took 2 s, because the files were still in the page cache.
- **`egdb_gen manifest`** (`logs/manifest_after.txt`): all 561 files ok (checksums match). That is 414 earlier files
  (2–7 pieces WLD and MTC) plus 147 new ones.
- **`egdb_test --data`** (`logs/egdb_test_data_after.txt`): passes, with database `max_pieces 8` and
  `max_mtc_pieces 6`.

## CheckerBoard engine with 8 pieces

`scripts/egdb8_dll.sh`, log `logs/egdb8_dll.out`:
- **Setup:**
  - DLL `engines/regicide/30f09c3-egdb8`: the same `regicide_cb.dll` as the 7-piece baseline (30f09c3, n4c FT 1024).
  - Its `egdb/` folder is a full copy of the 2–8 piece database on local NTFS: 561 files, 11 GB, copied in 69 s, and
    `diff -rq` identical to the source.
- **Through `cbhost.exe`:**
  - `get dbpieces` → **8**;
  - `get dbpath` → the DLL's `egdb` folder.
  - The first command, which loads the database, finishes in 0.17–0.25 s.
- **Cache:**
  - The DLL calls `egdb::init(<dll dir>/egdb, 4096)` at its first command.
  - `init` pre-faults every file only when the total fits in `cache_mb`. At 11 GB it does not, so the files are
    mapped and paged in on demand: no slow start, and the OS page cache, shared by all engine processes, warms up
    over the first games.
  - `set dbmbytes` applies only to a later `set dbpath`, and a database already in use stays in use, so it cannot
    change this after load.
  - With 2–7 pieces (about 1 GB), the same DLL pre-faulted everything. This is the one difference in setup between
    the 7- and 8-piece baselines besides the database itself.

## Files

- `logs/`:
  - `gen8_44.log`, `gen8_53.log`, `gen8_rest.log`: egdb_gen output;
  - `*.time.txt`: `/usr/bin/time -v`;
  - `status.txt`: timeline;
  - `manifest_before.txt`, `manifest_after.txt`, `verify8.txt`, `egdb_test_data_before.txt`,
    `egdb_test_data_after.txt`;
  - `files_new.txt`, `sizes_new.txt`: the 147 new files and their sizes;
  - `egdb8_dll.out`.
- `scripts/`: `prereq.sh`, `egdb8_chain.sh`, `egdb8_dll.sh`, `baseline_egdb8.sh`, `pausekeeper4.sh`.
