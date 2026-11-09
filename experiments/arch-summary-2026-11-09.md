# Arch laptop: state on 2026-11-09 (before the pause)

The laptop is an i9-13900H (6P+8E, 20 threads), 62 GiB RAM, Arch Linux, clang 23.1.4. GCC 16 cannot build the tree
(`__builtin_bitreverse32`), so build with `CC=clang CXX=clang++`. Long jobs run as transient **systemd user services**
(`systemd-run --user …`): they survive T3/Claude restarts, which a `setsid nohup` job inside the app's cgroup does not.
Each job is wrapped in `systemd-inhibit --what=sleep:idle:handle-lid-switch` at nice 10. Under 18-thread load the
package stays around 63 °C, and the `balanced` firmware profile caps the clocks at ~1.6 GHz P / ~1.1 GHz E.

## Data that lives only on this laptop

| what | path | size | in git? |
|---|---|---|---|
| **gen5b training data** (50k nodes, seeds 71+72, 769ef0b, 86,722 games, 8,293,909 positions) | `~/regicide-data/train/gen5b_n50k.bin` | 132,702,544 bytes; sha256 `c0dd2900ac16b52bc7d516b85f978b3a057df16f19f5035bfe44f979ce68513c` | **no**; only the README is, on t3/gen5b. Still to be transferred to Windows. |
| **2-7 piece EGDB** (`egdb_gen build --max 7 --mtc 6`, 635d949) | `~/regicide-data/egdb/` (414 files) | 1,002 MB | no, but rebuildable in ~19 min; the manifest is on t3/avoid-v1 (`experiments/avoid-v1/egdb_manifest_laptop.txt`) |
| deep book over all 156 acf156 ballots (2,532 positions, 30M nodes, ff0a3d2) | `~/regicide-data/book/book_deep_acf156.bin` | 230,492 bytes | yes, t3/book-deep-acf156 |
| book_w03 (40k expansions, w_lr0.03_wd0 eval, a44591e) | `~/regicide-data/book/book_w03.bin` | 3,975,804 bytes | yes, t3/book-w03 |
| avoid list v1 (79 entries) | `~/regicide-jobs/avoid1/avoid_v1.bin` | 1,280 bytes | yes, t3/avoid-v1 |

Job directories with logs, pinned binaries and scripts are under `~/regicide-jobs/` (book, tune1, avoid1, gen5b,
bookdeep, tune2), about 20 MB in total.

## Branches pushed from this laptop (none merged by me; master was never pushed)

| branch | content |
|---|---|
| t3/book-w03 | book_w03 + README |
| t3/tune1 | tune1 series 1-3 (RESULTS.md); its winners se_depth 4 and se_margin 20 are already in master |
| t3/avoid-v1 | avoid_v1.bin, lossprobe report, sanity check |
| t3/gen5b | gen5b README (counts, timing, sha256; the data file is local) |
| t3/book-deep-acf156 | deep acf156 book, roots, per-ballot root values, logs |
| t3/tune2 | tune2 on 6bb8c6c (RESULTS.md), this summary |

The worktrees are next to the main checkout: `~/Documents/Projects/Regicide-{bookw03,tune1,avoid1,gen5b,bookdeep,tune2}`.
The main checkout `~/Documents/Projects/Regicide` was left untouched on master (a44591e).

## tune2: state and how to resume

**State at 20:41: finished and stopped.** All 8 tune2 screens on base 6bb8c6c are done (300 ms × 800 pairs each).
None qualified for a 1 s confirmation, so the defaults stay. The best was `lmr_base=80` at +3.7 [−1.1, +8.4]. See
`experiments/tune2/RESULTS.md`. The `regicide-tune2` service has exited, `~/regicide-jobs/tune2/STOP` exists, and
nothing is queued. No regicide services or sleep inhibitors are left on the laptop.

- Runner: `~/regicide-jobs/tune2/runner.sh`, user unit `regicide-tune2`. It plays the queue
  `~/regicide-jobs/tune2/queue.txt` (`name|A params|movetime ms|pairs|B params|extra selfplay args`) in order, skipping
  lines listed in `done.txt`. Each test logs to `logs/<name>.log`, and a `DONE` line with the final result goes to
  `runner.log`. It exits after the current test when `~/regicide-jobs/tune2/STOP` exists.
- Binary: `~/regicide-jobs/tune2/bin/selfplay-6bb8c6c` (pinned, built from `Regicide-tune2/build-tune`).
- selfplay cannot resume a match: an interrupted test is lost and must re-run from scratch. If it was interrupted,
  delete its log, and make sure its line is not in `done.txt`.
- **To resume** (add tests to `queue.txt` first if wanted):

```sh
U=~/regicide-jobs/tune2; rm -f $U/STOP
systemd-run --user --unit=regicide-tune2 --collect -p Nice=10 -p WorkingDirectory=$U \
  -p StandardOutput=append:$U/runner.log -p StandardError=append:$U/runner.log \
  systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide tune2" $U/runner.sh
```

- To update the results file, run `cd ~/regicide-jobs/tune2 && python3 -I gen.py` (it writes
  `Regicide-tune2/experiments/tune2/RESULTS.md`), then commit and push t3/tune2.
- If master moves, rebuild in a new worktree, pin a new selfplay binary, point the runner at it, and start a new
  section in RESULTS.md (the base matters: tune1 showed parameters interact with the net and with SE).

- Copies of the runner, queue, done list and results generator are in `experiments/tune2/scripts/` (the live copies are in `~/regicide-jobs/tune2/`). The gen5b file has a local backup at `~/regicide-data/backup/gen5b_n50k.bin` (same sha256), on the same disk, so a copy on another machine is still needed.
