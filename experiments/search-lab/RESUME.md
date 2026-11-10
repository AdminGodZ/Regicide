# search-lab: how to resume (machine stopped for an update/restart on 2026-11-10 ~21:40 CET)

## State when stopped

- **Net matches** (`~/regicide-jobs/netm`, runner v2 `runner2.sh`, unit `regicide-netm`):
  - M5 n8c vs n8bn (db on): **complete** (1,610/1,610), −1.7 [−5.4, +1.9]. In RESULTS.md.
  - M1 n8q vs n6c (db on): **complete** (1,610/1,610), +3.5 [+0.1, +6.8]. In RESULTS.md.
  - M3 n8q vs n6c (db **off**): **interrupted at 391 of 1,610 games** (every chunk mid-run, exit 143). Interim
    −6.3 [−14.3, +1.7] over 194 pairs, labelled interim in RESULTS.md (results.csv of the partial chunks in
    `netm/matches/M3_interim/`). **All six chunks must be replayed** (runner v2 deletes and re-runs incomplete chunks).
  - M6a w2064 vs n8bn (db on): not started.
  - M2, M4, w2032, w1064: dropped by the lead.
- **LMR sweep** (`~/regicide-jobs/searchlab`, `runner.sh`, unit `regicide-searchlab`): not started. Queued 9 lines at
  the end of `~/regicide-jobs/searchlab/queue.txt` (lmr_base 20/40/80/100/120, lmr_mult 25/35/55/65; 200 ms exact,
  800 pairs, UHO set, 2–7 database, binary `selfplay-lab4`). It was to be started by `chain.sh` once M6a was done.
- **Services:** all were transient units (`systemd-run --collect`): `regicide-netm`, `regicide-netm-restart`,
  `regicide-chain`, `regicide-searchlab`. All stopped; there are no unit files, so nothing restarts by itself.
  `STOP` files exist in `~/regicide-jobs/netm` and `~/regicide-jobs/searchlab` (remove them to run).

## Resume commands

```sh
# 1. Net matches: M3 (replays all six chunks), then M6a. Runner v2 skips M5/M1 (they validate as complete).
N=~/regicide-jobs/netm; rm -f $N/STOP
systemd-run --user --unit=regicide-netm --collect -p Nice=10 -p WorkingDirectory=$N \
  -p StandardOutput=append:$N/runner.log -p StandardError=append:$N/runner.log \
  systemd-inhibit --what=sleep:idle:handle-lid-switch --why="regicide net matches" $N/runner2.sh
# watch: grep -E '^(BEGIN|DONE|INCOMPLETE|FAILED)' $N/runner.log ; python3 -I $N/validate.py $N/matches/<tag> --pooled
# post a finished match: $N/netpost.sh <tag> "<label>" <on|off>   (appends the row, commits per-game files, pushes)

# 2. LMR sweep after M6a: chain.sh waits for M6a in done.txt, stops the net runner, starts the sweep runner.
systemd-run --user --unit=regicide-chain --collect ~/regicide-jobs/searchlab/chain.sh
# (or directly, once the net runner is idle: rm -f ~/regicide-jobs/searchlab/STOP and start runner.sh as in chain.sh)
# results: python3 -I ~/regicide-jobs/searchlab/row.py lmr_base20 ... ; post with ~/regicide-jobs/searchlab/post.sh
```

Confirmations of any sweep candidate (≥ +5, lower bound > 0): 600 ms, 800 pairs, the other half of the UHO set:
add `--skip 800` to the extra args (e.g. `lmr_base80_conf|selfplay-lab4|lmr_base=80|600|800||--egdb ... --book
books/uho1600.txt --skip 800`).

## Lives only on this disk (outside the repository)

| what | path | rebuild |
|---|---|---|
| net-match engine (master 456e9b2 build: `libregicide_cb.so`, `match`, `cbhost`) | `~/regicide-jobs/netm/eng/`, `eng_nodb/`, `bin/` | worktree `~/Documents/Projects/Regicide-m456` (detached 456e9b2), `cmake --build build`; copy the .so into both eng dirs |
| egdb link | `~/regicide-jobs/netm/eng/egdb -> ~/regicide-data/egdb` | `ln -sfn ~/regicide-data/egdb ~/regicide-jobs/netm/eng/egdb` (eng_nodb must have none) |
| 2–7 piece database | `~/regicide-data/egdb/` (1 GB) | `egdb_gen build --max 7 --mtc 6` (~19 min) |
| nets | `~/regicide-jobs/netm/nets/` (n6c, n8bn, n8c, n8q, n8bn2, w1064, w2032, w2064) | from origin/t3/truth-data (runner fetches missing ones) |
| chunk files, openings4, scripts | `~/regicide-jobs/netm/` | copies in `experiments/search-lab/netm/` |
| match outputs (full PDN, logs) | `~/regicide-jobs/netm/matches/` | finished matches committed under `netm/matches/` |
| selfplay binaries | `~/regicide-jobs/searchlab/bin/` (`selfplay-lab4` = branch with `--skip`; `posbench`, `unbalanced`) | build t3/search-lab (`~/Documents/Projects/Regicide-searchlab/build`) |
| UHO set and other books | `~/regicide-jobs/searchlab/books/` | copies in `experiments/search-lab/books/` |
| n4c net for Part A | `~/regicide-jobs/searchlab/n4c.nnue` | `experiments/nnue4/n4c.nnue` |
