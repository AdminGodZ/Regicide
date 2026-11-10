# RESUME: truth-data on the Windows box (stopped 2026-11-10 ~21:30 for a machine restart)

## State at the stop

- **All my jobs are stopped:**
  - processes: both E5 generators, the loss analysis, the pausekeeper, the n9h trigger; nothing trains;
  - `~/regicide-jobs/locks/pk_hold` is removed, so `pausekeeper_truth.sh` does not run until it is recreated;
  - nothing of mine starts by itself after a reboot (no cron or systemd units);
  - I hold no `timing.lock`, and there is no `matchlock`.
- **E5 files** (`~/regicide-data/truth/`, whole records, checked after the stop):

| File | Records | Runs (seed) | Binary |
|---|---|---|---|
| `gen6a_n50k_egdb.bin` | 7,875,584 | 611 (11:25), then 614 (17:43) | 614: fixed binary sha256 df0a5f57… (branch ba45aca) |
| `gen6b_n50k_egdb.bin` | 6,227,456 | 612 (08:03), 613 (16:34), then 615 (17:42) | 615: fixed binary df0a5f57… |

- **Counts at the stop:** records 14103040, games 523378; distinct: 10973587 records in 423813 games (77.8%) (game-deduplicated = records left after removing exactly repeated games; unique board+side keys measured at ~19:25: 7,334,907 of 10,748,672).
- **Data from before the fixes** (seeds 611–613) has 19–23% repeated games. Always train on game-deduplicated copies:
  `python experiments/truth-data/scripts/dedupe_games.py OUTDIR gen6a… gen6b…`. `n9.sh` does this itself.

## Restart the generators (fresh seeds, fixed binary)

```
# inside WSL, from the branch build ~/regicide-truth/build (rebuild first if needed: cmake --build build -j 4)
touch ~/regicide-jobs/locks/pk_hold
bash /mnt/c/regicide-match/jobs/truth/launch.sh pausekeeper_truth.sh
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 616 6
bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 617 6
```

- `e5_gen.sh` copies `build/datagen` (it fails if the copy fails) and appends.
- datagen refuses partial-record files and offsets its random streams by the records already in the file, so a restart
  cannot replay games. Use new seeds anyway (next: 616, 617).
- Thread budget: base 8 (gen6a 6 + 2 for training); gen6b's 6 threads are the yield group, which stops while
  `yield_truth` exists or Book Lab computes (`bookgen-*`, `lossprobe*`, `blk-*`).

## Still open (lead's phase 3)

1. **n9h:** at 15M game-deduplicated E5 records, or at 08:00 on 2026-11-11 with what is there:
   `bash /mnt/c/regicide-match/jobs/truth/launch.sh n9h_trigger.sh` (or `n9.sh n9h` directly).
   - Recipe: n8q's (E2 files + gen5a_e2 ×3, K 102.4, λ 0.5, groups 2–8 / 9–24 = 1 : 9) + deduplicated gen6a/gen6b ×3.
   - Then the export check, sanity table, the draw proxy at 9–12 pieces (`tools/pilot_draws.py`), the E5 validation
     overlap and the embed test. Put the net on the branch; the laptop plays it against n8q.
2. **n9a:** the same at 30M (`n9.sh n9a`).
3. **Forecast** (not a measurement): ~2.0M game-deduplicated records per hour with both generators (~1.0M with gen6b
   paused). From the counts above, 15M is ~1 h of running away and 30M ~8–9 h.
4. **Loss analysis (item 4b):** 19 of 25 runs done (partial table in RESULTS.md).
   - Remaining: `ba-off-769-cake`, `ba-off-769b-cake`, `ba-off-769-2s-cake`, `ba-tm-off-cake`, `ba-deep-off-guinn`,
     `ba-off-769-guinn`.
   - Resume: `bash /mnt/c/regicide-match/jobs/truth/launch.sh losswalk.sh` (it skips finished `lw_*.tsv`).
   - Then `python /mnt/c/regicide-match/jobs/truth/tools/lossreport.py > …` and update RESULTS.md (section 4b).
   - The five n8q games are done (RESULTS.md, 4b).
5. **Pending elsewhere:** the laptop's 0.5 s net matches (origin/t3/search-lab) and the Book Lab verdict on n8q with the
   new book; bd1c5c0 (n8q built in) stays unmerged.

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
  - `~/regicide-jobs/truth/` (status, `lw_*.tsv`, `pilot*.csv`, logs);
  - job scripts in `C:\regicide-match\jobs\truth` (copies in `experiments/truth-data/scripts/jobs/`);
  - helper scripts in `C:\regicide-match\jobs\truth\tools`.
- **Windows DLL:** `C:\regicide-match\engines\regicide\truth-c0dde2e` (conversion search; egdb junction).
- **Worktree:** `~/regicide-truth` (branch t3/truth-data); build dirs `build/` (n8q built in), `build-ft2048/`, `build-win/`.
