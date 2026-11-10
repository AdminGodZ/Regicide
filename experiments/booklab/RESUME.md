# Resume after the 2026-11-10 21:30 stop (owner restart of Win11-AdminGod)

## State at the stop

- Nothing of booklab was running at 21:29:31: the WP13 chain had just stopped by itself (see below), chain2 had
  stopped at 21:26 (hold), the WP12 scoring job had been stopped at ~21:00, the old WP7 script `t_w1.sh` was killed.
  No cron jobs, services or loops of booklab exist; the timing lock was free; no cbhost process of booklab was left.
- **WP13 evidence, Cake as Black** (`bl-w13-ev-cake`, KingsRow mode 3 as White, 40 games = 5 ballots × 8): **all 40
  games were played** (40 drawn) before the stop. The wrapper marked it `INCOMPLETE` only because the current runner
  reports "Games: 40 of 80 scheduled" for a fixed-colour (A=W) deck. Its `exit_status` was set to 99 at the stop, so
  a rerun of `wp13_chain.sh` replays it. Records: `wp13/games/bl-w13-ev-cake.pdn.gz` (usable as evidence; label it
  "first attempt" if replayed). To do: fix `done_run` in `scripts/bl_run_w.sh` for fixed-colour decks (count the
  deck's lines with A=B/A=W as one game each) before replaying anything with such decks.
- Not started: WP13 evidence with GuiNN and KingsRow mode 3 as Black, the Cake vs KingsRow mode-3 census.
- Block 4 (E vs EB on H2x) is partial: finished and trustworthy `w10-b4-E-cake-H2x_0`, `w10-b4-EB-cake-H2x_0`,
  `w10-b4-E-kingsrow-H2x_0`, `w10-b4-EB-kingsrow-H2x_0` (the KingsRow parts in book mode 3); the rest is held by
  `~/regicide-jobs/booklab/HOLD_TAGS` (`^w10-b4-`, `^v11-`, `^w10-b5-`). Block 5 dropped for now.

## Complete and trustworthy (pushed)

- KingsRow mode 3 on the production deck with r10 (`k3/`): 2/307/3 and 1/307/4; first attempt of a-h1 (16 crashes)
  discarded and replayed. Loss analysis `k3/k3_losses.md`; KingsRow mode-3 answers on the loss lines
  `k3/kr_defence.tsv` and `wp13/kr_defence.tsv`.
- Book-mode probe (`data/book_probe.log`), runner-accounting measurement (`data/acct_report.md`, clock-corrected;
  `data/acct_report_uncorrected.md`), the WSL clock finding (WSL monotonic 7.9 % slow against Windows QPC), Block 3,
  everything before.
- Review fixes up to 717c716 (wrapper: lock before pre-flight, marker before unlock, hold file; pins; cbshim report).

## Partial / draft (labelled)

- `wp13/repairs_k3_draft/`: theory-table repairs from the mode-1-mixed table (#141/#9: 6-9 at ply 6; #34: 7x16 at
  ply 4; #60, #71: none). Draft only: WP13 must rebuild the theory table from games against KingsRow mode 3 only.
- `wp12/` tools are fixed for M1–M4 but four reviewer residuals are open (lead message 21:35): t1_report bank from
  the runner's own value (log it), wp12_discover must not replay a quarantined game, wp12_select DB key by content
  checksums, wp12_select cap/spacing on every affected line with each context's ply.

## Resume, in the lead's order

All commands run inside WSL (`wsl.exe`, user admingod) from any directory; job scripts live on
`/mnt/c/regicide-match/jobs/booklab/` (copies in `experiments/booklab/scripts/`). Start each with
`nohup setsid bash SCRIPT > LOG 2>&1 < /dev/null &`.

1. Wrapper fix for fixed-colour decks (`done_run` in `bl_run_w.sh`, see above); keep `HOLD_TAGS` as it is.
2. WP13 evidence: `bash /mnt/c/regicide-match/jobs/booklab/wp13_chain.sh` (replays w13-ev-cake, then GuiNN and
   KingsRow2 as Black, then the Cake vs KingsRow mode-3 census `c3-cake-kingsrow-h1/h2`).
3. WP13 repair: theory table from `bl-k3-kingsrow-*` and `bl-w13-ev-*` only
   (`tools/booklab/theory.py build --db ~/regicide-jobs/booklab/theory_w13.sqlite --replay
   ~/regicide-booklab/build-linux/booklab_replay --list LIST`), then `tools/booklab/repair_all.py --db
   theory_w13.sqlite --out repairs_w13 [--opp SLUG=FAM] ~/regicide-match/results/bl-k3-kingsrow-*`; decisions =
   `decisions_r10.txt` + repairs; `make_book.sh` → `book_r10k.bin`; `bl_build_dll.sh 456e9b2-r10k book_r10k.bin 6
   456e9b2`.
4. Targeted tests of r10k: Regicide Black on ACF #60, #71, #34, #141, #9 against KingsRow mode 3 ×10, Cake ×4,
   GuiNN ×4 (fixed-colour decks `| moves | A=B`, `MATCH=bin/match-cur EXTRA_ARGS=--allow-rejected-options`); iterate.
5. Block 7: EB (`bl-456e9b2-r10`) against KingsRow mode 3 on H1, H2, H3_0, H3_1 (tags `w10-b7-EB-kingsrow-<part>`,
   current runner; parts in `~/regicide-jobs/booklab/wp10/parts/`).
6. Two production-deck runs of r10k against KingsRow mode 3 (`bl_run_x.sh regicide:bl-456e9b2-r10k kingsrow
   k3r10k-{a,b}-{h1,h2} /mnt/c/regicide-match/jobs/booklab/acf156_{h1,h2}.txt`).
7. New pins on r10k from all validated games (`tools/booklab/pins.py --runs` final Cake/GuiNN runs, mode-1 KingsRow
   runs, the two r10k mode-3 runs `--book book_r10k.bin`), → new r11; DLL.
8. WP11 validation (remove `^v11-` from HOLD_TAGS; r11 with n6c and n8q against all three, KingsRow mode 3).
9. T1 (`t1_run.sh`; first decide the clock question in RESULTS/messages; fix t1_report's bank first).
10. WP12 pilot (points from the v11-n6c-cake games; the four residual fixes first).

## Only on this machine (outside the repository)

- `~/regicide-jobs/booklab/`: status log `status`, books (`book_master.bin`, `book_r10.bin`, `book_r11.bin`),
  decisions (`decisions_r10.txt`, `decisions_r11.txt`), theory tables (`theory.sqlite`, `theory_k3.sqlite`), WP10
  parts (`wp10/parts/`), WP12 work (`wp12/`, incl. `s20m/scores.tsv`: 423 points at 20 M nodes, old format,
  migratable), accounting shim logs (`acct/`), `HOLD_TAGS`, binaries `bin/` (match-cur, match-pr49, blk-*).
- `~/regicide-match/results/bl-*`: all raw result directories (game records are committed gzipped where reported).
- `C:\regicide-match\engines\regicide\bl-*` (DLLs; `egdb` junctions to `6bb8c6c-deep2b-egdb8\egdb`),
  `C:\regicide-match\bin\pr49-c4fb8fd\cbhost.exe`, `~/regicide-match/builds/` (build trees, pr49 worktree).
- `/mnt/c/regicide-match/jobs/booklab/` (job scripts and the staging copy of the tools).
- Worktree `~/regicide-booklab` (branch t3/booklab).
