# Baseline: Regicide with NNUE n4c (FT 1024) vs KingsRow, Cake, GuiNN

## Engine

The engine is master `eb1037c` built with `-DREGICIDE_NNUE_FT=1024` and `experiments/nnue4/n4c.nnue` embedded as
`src/weights/net.bin` (DLL `eb1037c-n4c`, `BUILD.txt`). Singular extensions start at depth 4. There is no opening book.

n4c is the NNUE round-4 winner (`experiments/nnue4` on `t3/nnue4`): **+5.6 [+0.9, +10.3]** vs nn3e at 0.1 s/move,
1,610 games.

**This is the same engine as master `30f09c3`** (default FT 1024, built-in n4c), which was merged while the baseline
was already running.
- A plain 30f09c3 Windows build has all loaded PE sections (`.text`, `.rdata`, `.data`, `.pdata`, `.tls`, `.reloc`)
  byte-identical to the baseline DLL. Only the PE timestamp, `.buildid` and the unloaded COFF symbol table differ.
- 30f09c3 bench (Linux and Windows) is 5,114,091, the same as this build.
- See `build_30f09c3_and_dll_compare.txt`.

Engine checks (`build_and_bench.txt`):
- `regicide.exe` from the same Windows build gives a bench of **5,114,091 nodes**. So do the Linux build of the same
  tree and the 385611e FT-1024 build with `set evalfile n4c.nnue`.
- The DLL embeds `net.bin` (n4c, 298,828 bytes) and `eval.bin`.
- Linux ctest passes 6/7. The failing check is `nnue`, and the failure is expected: in a custom-width build
  (`REGICIDE_NNUE_FT` set) the test assumes the built-in net is rejected, but here n4c fits and is accepted
  ("built-in evaluator has a net without net.bin"). 30f09c3, where FT 1024 is the default, passes 9/9.

## Setup

Exactly as `results/baseline-ae210fb`:
- **Openings:** `acf156` (156 ACF 3-move ballots, each played with both colours: 312 games).
- **Time:** `--movetime 0.5` (CheckerBoard engine-match rules), `-c 8`.
- **Regicide:** hashsize 128, threads 1, own 2–7 piece endgame database (copy on local disk).
- **KingsRow 1.20:** own book, 2–8 piece WLD database (`dbmbytes 512`), hashsize 128, searchthreads 1.
- **Cake 1.89f and GuiNN 2.06 (avx2):** own book (default), KingsRow 2–8 piece WLD database, `dbmbytes 512`, hashsize
  128.
- **Adjudication:** runner defaults; engine draw claims ignored.
- **Binaries:** `tools/match` (`match-747ab51`) and b71dfa9 `cbhost.exe`, unchanged since b71dfa9.
- **Scripts:** `run_baseline_r4.sh` (via `baseline_all.sh`), `build_win_variant.sh` (+ `win_n4c.sh`).

Machine load: Intel Core Ultra 9 285K, Windows 11 + WSL2.
- A 12-thread datagen ran alongside for the whole baseline, as in the earlier baselines: gen4a, this same engine
  (Linux build), seed 53, from 11:31:53.
- From 11:37 to 11:41, during the KingsRow run, a `nice`d 2-job build of 30f09c3 ran.
- Nothing else ran: no training and no other matches.
- `timeline.txt` contains a failed first launch at 11:31:33, which played 0 games because of a quoting bug in the DLL
  path. Its empty outputs were deleted, and the real runs start at 11:32:42.

## Results

| Opponent | W / D / L | Score | Elo [95% CI] (pentanomial) | ae210fb (nn2b) | Δ |
|---|---|---|---|---|---|
| KingsRow 1.20 | 0 / 308 / 4 | 49.36% | **−4.5** [−8.8, −0.1] | −2.2 [−5.3, +0.8] | −2.3 |
| Cake 1.89f | 0 / 298 / 14 | 47.76% | **−15.6** [−23.4, −7.8] | −22.3 [−31.5, −13.2] | +6.7 |
| GuiNN 2.06 | 3 / 303 / 6 | 49.52% | **−3.3** [−9.9, +3.2] | −10.0 [−18.3, −1.7] | +6.7 |

- **Pentanomial counts [LL LD DD/WL WD WW]:**
  - KingsRow [0 4 152 0 0];
  - Cake [0 14 142 0 0];
  - GuiNN [0 6 147 3 0].
- **Against Cake and GuiNN:** clearly better point estimates than ae210fb, with 6 fewer losses each. GuiNN is now
  within noise of even.
- **Against KingsRow:** 2 more losses than ae210fb (4 vs 2), which is −2.3 Elo. That is well within noise: there are
  only 2–4 decisive pairs in each run.
- **Colour:** all 4 KingsRow losses are as Black (Inferno, Wilderness I, Gemini I, Octopus). Cake: 11 of 14 losses as
  Black. GuiNN: 5 of 6 losses as Black; the wins came from Wilderness II, ACF #36 and ACF #112.
- **Run times:** KingsRow 11:32 to 11:57, Cake 11:57 to 12:19, GuiNN 12:19 to 12:42.

## Files

- Per opponent (`kingsrow/`, `cake/`, `guinn/`):
  - `summary.txt`: runner summary, pentanomial Elo;
  - `results.csv`: one row per game;
  - `losses.pdn`: every game Regicide lost, with per-move times and engine output as comments;
  - `runner.log`.
- `BUILD.txt`, `build_and_bench.txt`, `build_30f09c3_and_dll_compare.txt`, `n4c.nnue` (the embedded net).
- `timeline.txt`: status lines for the baseline runs and the datagen.
- The 2 s/move KingsRow run (same engine and settings, `--movetime 2`) will be in `kingsrow-2s/`.
