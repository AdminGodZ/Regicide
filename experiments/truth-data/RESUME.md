# RESUME: truth-data on the Windows box (stopped 2026-11-11 16:39 for the night, lead's wind-down)

## State at the stop

- **Nothing of mine runs.** Stopped at 16:39:
  - both E5 generators, killed with TERM + CONT, files cut to whole records (0 bytes cut);
  - the pausekeeper: `~/regicide-jobs/locks/pk_hold` removed;
  - the keepalive: `~/regicide-jobs/truth/hold` removed;
  - the n9a0 waiter (`n9a_variant.sh`), before it started anything.
- The n9a trigger had already finished: it ran n9a. No cron or systemd units. I hold no `timing.lock`, and there is
  no `yield_truth`, `matchlock` or `pk_hold`.
- **Nothing is half-written:** no training ran at the stop, and no `.tmp` files exist.

### Data files (`~/regicide-data/truth/` unless noted)

| File | Records | Labels | Notes |
|---|---|---|---|
| `gen6a_n50k_egdb.bin` (E5) | 25,111,296 | database results at generation (datagen --egdb; no ≤ 8-piece records, so the relabel defect does not apply) | GAME_START flags. Random openings with 4–12 plies before record 12,514,816, 6–16 from there |
| `gen6b_n50k_egdb.bin` (E5) | 9,334,784 | as gen6a | 4–12 plies before record 6,659,840, 6–16 from there |
| `e5_n9h/gen6{a,b}_n50k_egdb_dedup.bin` | 10,775,453 / 4,360,885 | as above, game-deduplicated | n9h's snapshot (04:10: raw 12,435,456 / 6,659,840) |
| `e5_n9a/gen6{a,b}_n50k_egdb_dedup.bin` | 23,021,213 / 7,035,829 | as above, game-deduplicated | n9a's snapshot (16:13: raw 24,681,216 / 9,334,784) |
| `*_e2.bin` (gen1…gen4a, gen5a, gen5c) | as the originals | relabelled with the **defective** rule (whole game = first covered value) | n8q and n9h were trained on these |
| `*_e2f.bin` (same twelve) | as the originals | relabelled with the **fixed** rule (every ≤ 8-piece record its own value), checked 0 wrong | n9a trains on these |
| `~/regicide-data/train/*.bin` | — | original self-play results | sources of E2/E2f |

- E5 at the stop: 34,446,080 raw records. Game-deduplicated: 30,056,530 at 16:13, when raw was 34,015,232.
- Since the 04:15 restart no game repeats an earlier game or start: 0 of 95,866 at 06:39.

### Nets

- **n9a: complete.** 40 of 40 epochs; final valid 0.413548 float / 0.413426 int.
  - Files: `~/regicide-truth-work/n9a.nnue` (sha256 297a1eddca7106b9…, marker `n9a.ok`), checkpoints
    `~/regicide-truth-work/runs/n9a/{last.pt,ckpt_e040.pt,…}`, log `runs/n9a.log`.
  - Recipe: n8q's on the **E2f** files + gen5a_e2f ×3 + n9a's E5 dedup ×3.
  - Checks done: export check OK, sanity (`sanity_n9a.md`), draw proxy (`pilot_draws_n9a.md`), validation overlap
    (`unique_keys_n9a.txt`), embed test ctest 54/54 (bench 6,613,867 with n9a built in).
  - Net and logs are committed (`experiments/truth-data/n9a.nnue`, `logs/*n9a*`). **Not yet written up in RESULTS.**
- **n9a0** (n9a without the 6–16-ply data, the lead's optional comparison): **not started**; nothing on disk.
- n9h: committed and written up; the laptop match n9h vs n8q was running (interim +3.9 [−1.0, +8.7] after 402 pairs).

## Commands to continue (inside WSL)

```
# keepalive (from PowerShell): Start-Process -WindowStyle Hidden wsl.exe -ArgumentList 'bash','/mnt/c/regicide-match/jobs/truth/keepalive.sh'
touch ~/regicide-jobs/locks/pk_hold
bash /mnt/c/regicide-match/jobs/truth/launch.sh pausekeeper_truth.sh
# generators (only if more E5 data is wanted): verified binary, next seeds 622 / 623, 6-16 random plies
RANDOM_PLIES=6-16 EXPECT_SHA=a99a1334887d2434 bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6a_n50k_egdb datagen-tr6 622 6 0 gen6b_n50k_egdb
RANDOM_PLIES=6-16 EXPECT_SHA=a99a1334887d2434 bash /mnt/c/regicide-match/jobs/truth/launch.sh e5_gen.sh gen6b_n50k_egdb datagen-tr5 623 6 1 gen6a_n50k_egdb
# n9a0 comparison (starts at once: "n9a DONE" is in the status file; ~30 min GPU + checks)
bash /mnt/c/regicide-match/jobs/truth/launch.sh n9a_variant.sh
```

- `e5_gen.sh` with `EXPECT_SHA` uses the installed binary only if its sha256 matches. `--avoid` (both files) and
  parity 0 / 1 keep every start unique.
- To restart a running pair: `RANDOM_PLIES=6-16 bash restart_e5.sh a99a1334887d2434 SEED_A SEED_B`. It waits for
  the timing lock, stops the generators and relaunches them.
- Data from before the 02:38 fix has repeated games. Always train on game-deduplicated copies (`n9.sh` does this).

## Next

1. Write up n9a in RESULTS from its logs. Add the start-kind shares of n9a's E5 data (`truth startkinds`) and the
   seeded-start validation loss of n8q / n9h / n9a (`scripts/seeded_loss.py`).
2. n9a0 comparison (`n9a_variant.sh`): draw proxy and seeded-start loss with and without the 6–16-ply part.
3. Put n9a to the laptop match against n8q (and n9h). Note that n9a differs from n9h in two ways: corrected E2
   labels and twice the E5 data. A cheap control that separates them would be n9h's recipe on E2f (~30 min).
4. bd1c5c0 (n8q built in on this branch) is superseded by master PR #64.

## Only on this machine (not in the repository)

- **Training data:** see the table above; plus `*_e3.bin` (E3, dropped), `tails/` (held-out proven sets) and
  `seeds_real_6_30.txt` (E5 seeds).
- **Database:** `~/regicide-data/egdb` (2–8 pieces, 11 GB).
- **Nets, checkpoints and logs:** `~/regicide-truth-work/` (`*.nnue` with `*.ok`, `runs/<net>/`).
- **Job state:**
  - `~/regicide-jobs/truth/` (status, logs, `k3/`, `restart_e5.log`, `restart_offset_*.txt`);
  - job scripts in `C:\regicide-match\jobs\truth` (copies in `experiments/truth-data/scripts/jobs/`);
  - helpers in `C:\regicide-match\jobs\truth\tools`.
- **Generator binaries:** `~/regicide-jobs/truth/bin/datagen-tr5`, `datagen-tr6` (sha256 a99a1334887d2434…).
- **Worktree:** `~/regicide-truth` (branch t3/truth-data); build dirs `build/`, `build-m/` (not committed),
  `build-ft2048/`, `build-win/`.
- Disk: 862 GB free.
