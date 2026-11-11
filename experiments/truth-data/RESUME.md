# RESUME: truth-data on the Windows box (running again since 2026-11-10 23:48; updated 2026-11-11 04:45)

## What runs (all started by hand after the restart; nothing restarts by itself after a reboot)

| Job | Started | Notes |
|---|---|---|
| keepalive (`keepalive.sh`, hidden `wsl.exe`) | 11-10 23:4x | keeps the WSL VM up; `~/regicide-jobs/truth/hold` |
| `pausekeeper_truth.sh` | 11-10 23:4x | runs while `~/regicide-jobs/locks/pk_hold` exists; yield group also stops while `timing.lock` is held or Book Lab computes (`bookgen-*`, `lossprobe*`, `blk-*`) |
| E5 `gen6a_n50k_egdb` | 11-11 04:15 | `datagen-tr6`, seed 620, parity 0, `--avoid` both files, **random plies 6–16**, 6 threads (base budget), sha256 a99a1334887d2434… |
| E5 `gen6b_n50k_egdb` | 11-11 04:15 | `datagen-tr5`, seed 621, parity 1, `--avoid` both files, random plies 6–16, 6 threads (yield group), same binary |
| `n9a_trigger.sh` | 11-11 01:06 | runs `n9.sh n9a` (n9h is done) once 30M game-deduplicated (exact check from 33M raw, every 20 min) |
| `n9a_variant.sh` | 11-11 06:50 | after "n9a DONE": trains n9a0 (n9a without the 6–16-ply data), then draw proxy, seeded-start loss, start kinds |

- n9h is done (61e9802), and the loss analysis is done.
- Seeds used so far: gen6a 611, 614, 616, 618, 620; gen6b 612, 613, 615, 617, 619, 621. Next free seeds: 622, 623.
- Random plies of the random-opening half are 6–16 from gen6a record 12,514,816 and gen6b record 6,659,840 on.
  Before that they were 4–12. Restarts must keep `RANDOM_PLIES=6-16`.
- Since 02:38 no start is played twice: `--avoid` preloads both files' starts, and parity 0 / 1 keeps the generators
  apart. Measured 03:08: 0 of 22,834 new games repeat anything.
- To restart both generators: `RANDOM_PLIES=6-16 bash restart_e5.sh a99a1334887d2434 622 623` (in WSL, from
  `/mnt/c/regicide-match/jobs/truth`, or through `launch.sh`). The script:
  - waits for the timing lock;
  - stops both generators (TERM + CONT, also their stopped parents);
  - keeps whole records;
  - installs the binary with that checksum (the installed one, else `build/datagen`);
  - relaunches with `--avoid` and parities, logging the offsets to `restart_e5.log`.
- 00:50: raw E5 records 15,332,352 (gen6a 8,978,176, gen6b 6,354,176).
- Done since the restart:
  - k3 analysis: RESULTS.md 4b, "KingsRow, book mode 3", data in `data/k3/` (02687c2).
  - master 235d3e0 merged, pilot early check and ctest (b0c59d4). `build/` was rebuilt on it at 00:49 (bench 7,833,556).

## Restart after a stop (fresh seeds, the verified binary)

```
# inside WSL
touch ~/regicide-jobs/locks/pk_hold
bash /mnt/c/regicide-match/jobs/truth/launch.sh pausekeeper_truth.sh
RANDOM_PLIES=6-16 EXPECT_SHA=a99a1334887d2434 bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 622 6 0 gen6b_n50k_egdb
RANDOM_PLIES=6-16 EXPECT_SHA=a99a1334887d2434 bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 623 6 1 gen6a_n50k_egdb
bash /mnt/c/regicide-match/jobs/truth/launch.sh n9a_trigger.sh     # if n9a is not done yet
# keepalive, from PowerShell:
# Start-Process -WindowStyle Hidden wsl.exe -ArgumentList 'bash','/mnt/c/regicide-match/jobs/truth/keepalive.sh'
```

- With `EXPECT_SHA`, `e5_gen.sh` uses the binary already in `~/regicide-jobs/truth/bin` only if its sha256 starts with
  the given prefix. Without `EXPECT_SHA` it copies `build/datagen`, and it fails if the copy fails.
- datagen refuses partial-record files and offsets its random streams by the records already in the file.
- Before a restart, check that nothing is left: `pgrep -a datagen`, pk_hold, matchlock, and whole-record file sizes.
- **Data from before the fixes** (seeds 611–613) has 19–23% repeated games. Always train on game-deduplicated copies:
  `python experiments/truth-data/scripts/dedupe_games.py OUTDIR gen6a… gen6b…`. `n9.sh` does this itself.

## Still open (lead's phase 3)

1. **n9h:** the trigger above. Recipe: n8q's (E2 files + gen5a_e2 ×3, K 102.4, λ 0.5, groups 2–8 / 9–24 = 1 : 9)
   plus deduplicated gen6a/gen6b ×3. Then:
   - the export check;
   - the sanity table;
   - the draw proxy at 9–12 pieces;
   - the E5 validation overlap;
   - the embed test.

   Then put the net on the branch for the laptop match against n8q.
2. **n9a:** the same at 30M game-deduplicated records (`n9.sh n9a`; trigger to be armed after n9h).
3. **Loss analysis (4b):** done (aac3fa6, horizon count corrected in 6bfdbaf).
   **Start pool:** widening the random openings (6–16 plies) is proposed in RESULTS (E5 repeats) and waits for the
   lead.
4. **Pending elsewhere:** the laptop's 0.5 s net matches and the Book Lab verdict on n8q; bd1c5c0 (n8q built in)
   stays unmerged.

## Only on this machine (not in the repository)

- **Training data:**
  - `~/regicide-data/train/*.bin`: original gen1…gen5c;
  - `~/regicide-data/truth/*_e2.bin`: database-relabelled;
  - `*_e3.bin`: rescored gen5a, gen4a (E3 is dropped);
  - `gen6a/gen6b_n50k_egdb.bin`: E5;
  - `e5_n9h/`: if created;
  - `tails/`: held-out proven sets;
  - `seeds_real_6_30.txt`: E5 seeds.
- **Database:** `~/regicide-data/egdb` (2–8 pieces, 11 GB).
- **Nets, checkpoints and logs:** `~/regicide-truth-work/`.
  - `*.nnue` with `*.ok` export markers;
  - `runs/<net>/{last.pt,ckpt_e*.pt}` for every n8*, w* net (needed to fine-tune or `--resume`);
  - `matches/`, `engines/rtruth.so` (fcf6acb), `engines/rft2048.so`, `engines-nodb/`.
- **Job state:**
  - `~/regicide-jobs/truth/`: status, `lw_*.tsv`, `pilot*.csv`, logs, `k3/`;
  - job scripts in `C:\regicide-match\jobs\truth` (copies in `experiments/truth-data/scripts/jobs/`);
  - helper scripts in `C:\regicide-match\jobs\truth\tools`.
- **Windows DLL:** `C:\regicide-match\engines\regicide\truth-c0dde2e` (conversion search; egdb junction).
- **Worktree:** `~/regicide-truth` (branch t3/truth-data).
  - Build dirs: `build/` (n8q built in), `build-m/` (merge check; not committed), `build-ft2048/`, `build-win/`.
