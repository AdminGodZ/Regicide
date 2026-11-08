# Book and avoid-list test: off / avoid / book+avoid vs KingsRow, Cake, GuiNN (master 769ef0b, 8-piece EGDB)

**Result: neither the avoid list nor the new NNUE book with the avoid list makes a measurable difference** at
0.5 s/move against any of the three engines. All differences are within ±2.2 Elo, far inside the CIs:

| Opponent | off | avoid (`avoid_v1`) | book+avoid (`book_n4c_avoid`) |
|---|---|---|---|
| KingsRow 1.20 | **−0.0** [−0.0, −0.0], 0/312/0 | **−1.1** [−4.9, +2.7], 1/309/2 | **−2.2** [−5.3, +0.8], 0/310/2 |
| Cake 1.89f | **−12.3** [−19.2, −5.3], 0/301/11 | **−12.3** [−19.2, −5.3], 0/301/11 | **−14.5** [−22.0, −6.9], 0/299/13 |
| GuiNN 2.06 | **−3.3** [−9.1, +2.4], 2/305/5 | **−3.3** [−9.9, +3.2], 3/303/6 | **−2.2** [−7.6, +3.1], 2/306/4 |
| mean | −5.2 | −5.6 | −6.3 |

Pentanomial counts [LL LD DD/WL WD WW]:

| Opponent | off | avoid | book+avoid |
|---|---|---|---|
| KingsRow | [0 0 156 0 0] | [0 2 153 1 0] | [0 2 154 0 0] |
| Cake | [0 11 145 0 0] | [0 11 145 0 0] | [0 13 143 0 0] |
| GuiNN | [0 5 149 2 0] | [0 6 147 3 0] | [0 4 150 2 0] |

## Why the avoid list changes so little

`avoid_hits.txt` replays every game (first 60 plies) and looks for the avoid-list positions, taken from the 66
distinct `BLAME` entries of `report_v1.txt`, with Regicide to move.

| Run | Avoid positions reached | Avoided move played |
|---|---|---|
| off: KingsRow / Cake / GuiNN | 3 / 19 / 12 | **0 / 6 / 3** (in 0 / 5 / 2 games) |
| avoid: KingsRow / Cake / GuiNN | 3 / 16 / 11 | **0 / 0 / 0** |
| book+avoid: KingsRow / Cake / GuiNN | 1 / 14 / 8 | 0 / 0 / 0 |

- **The list works:** with it, Regicide never played an avoided move.
- **The current engine rarely needs it.** The entries come from losses of older engines: 747ab51 (nn1), ae210fb (nn2b)
  and eb1037c-n4c with the 7-piece database. Without the list, today's engine (n4c, se_margin 20, 8-piece database) played
  an avoided move in only 7 of 936 games.
- So the list can change at most about 7 games per 936. That is consistent with 0 ± 2 Elo.
- **These matches use the very ballots** the entries were learned from, as intended. The list still has no measurable
  effect here, because the engine has moved past most of those mistakes.

## The book

- **`book_n4c_avoid.bin`** (`origin/t3/book-n4c`, `experiments/book-n4c`): 30,158 positions plus 79 avoid entries,
  embedded as `src/weights/book.bin`, default CheckerBoard book mode 3 (best moves only).
- **Usage:** Regicide played about 3.5 instant book moves per game against KingsRow (1,103 in 312 games) and about 4.2
  against Cake and GuiNN (1,300 each). These counts include a few instant root-database moves, which the DLL also
  reports as depth 0, nodes 0; the runs without a book show 0–161 such moves in total.
- **Effect:** none measurable: −2.2 / −14.5 / −2.2 against off's −0.0 / −12.3 / −3.3. Against KingsRow the book
  costs 2 lost games, against Cake 2 more. Against GuiNN the result is 1 game better.

## Setup

- **Engines:** all from master **769ef0b** (PR #20: EGDB cache fix, avoid-list support, bench 7,338,658, ctest 13/13;
  `build_769ef0b.txt`), Windows CB DLL built with llvm-mingw clang 23.1.5:
  - **off:** DLL `769ef0b-egdb8`, empty built-in book (so no avoid list), full 2–8 piece EGDB in its `egdb/` folder.
  - **avoid:** the same DLL with `--option-a "set avoidfile C:\regicide-match\avoid\avoid_v1.bin"`, which loads 79
    entries (`avoid_v1.bin` from `origin/t3/avoid-v1`).
  - **book+avoid:** DLL `769ef0b-bookavoid-egdb8`, the same tree with `book_n4c_avoid.bin` embedded
    (`build_769ef0b_bookavoid.txt`):
    - bench 6,636,877 with the book on and 7,338,658 with `set book off`;
    - through `cbhost`: `dbpieces` 8, book mode 3, built-in avoid list 79 entries;
    - plays 11-15 instantly from the start position.
  - Every `BUILD-*.txt` says "egdb: … (2-7 pieces)". That line is a stale template in `build_win_variant.sh`; the
    copies are the full 2–8 database (561 files, `diff -rq` identical, `dbpieces` 8).
- **EGDB setting:** the default (4096 MB cache, which in 769ef0b pre-faults the 2–7 set plus the smallest 8-piece
  slices). The EGDB speed re-check before the tests found the same nps with `set dbmbytes 12288`, so it was not used
  (`origin/t3/egdb8`, `experiments/egdb8/speed.md`).
- **Settings:** as the baselines: `acf156` (156 ballots × 2 = 312 games), `--movetime 0.5`, `-c 8`.
  - Regicide: hash 128, threads 1.
  - KingsRow: own book, 2–8 WLD database (`dbmbytes 512`), hash 128, searchthreads 1.
  - Cake and GuiNN: own books, the KingsRow 2–8 WLD database, hash 128.
  - Runner: `match-747ab51` with b71dfa9 `cbhost.exe`; script `scripts/run_ba2.sh` via `ba_chain2.sh`.
- **Order:** interleaved per opponent: off kingsrow, avoid kingsrow, off cake, avoid cake, off guinn, avoid guinn
  (19:09–20:28), then book+avoid kingsrow, cake, guinn (20:28–21:07).
- **Background load:** one generator at 10–12 threads ran alongside throughout, as in earlier baselines; no other
  matches or training ran.
  - Until 19:43 it was bookgen (`book_n4c`, nice 5, 10 threads). That covers off kingsrow, avoid kingsrow and off cake.
  - From 19:43 it was gen5a datagen (master 635d949, 50k nodes/position, 12 threads), for all other runs.
  - **So off vs avoid against Cake ran under different backgrounds.** A repeat of off vs Cake under gen5 is in
    `results/cake-analysis-769ef0b` on `t3/cake-analysis` (added when it finishes).

## Extra runs with master 65f7143 (`extra-65f7143/`)

Before the 769ef0b cache fix, the same test started with a 65f7143 DLL (same play; the EGDB loads without the partial
pre-fault). Two KingsRow runs finished, with bookgen in the background:

| Run | W/D/L | Elo |
|---|---|---|
| off | 1/307/4 | −3.3 [−8.2, +1.5] |
| avoid | 0/312/0 | −0.0 [−0.0, −0.0] |

They show the reverse order of the 769ef0b pair (off −0.0, avoid −1.1). Together the four runs put off and avoid
against KingsRow within noise of each other.

## 2 s/move KingsRow check

The better variant by mean Elo was **avoid** (−5.6 vs book+avoid −6.3), so it played KingsRow at `--movetime 2`
(`avoid-kingsrow-2s/`, 21:07–22:04, gen5a alongside):

| Run | W / D / L | Elo [95% CI] | Pentanomial |
|---|---|---|---|
| **avoid, 2 s, 769ef0b + 8-piece EGDB** | **0 / 312 / 0** | **−0.0** [−0.0, −0.0] | [0 0 156 0 0] |
| reference: n4c, no avoid, 2 s, 7-piece EGDB (`results/baseline-eb1037c-n4c/kingsrow-2s`) | 1 / 309 / 2 | −1.1 [−4.9, +2.7] | [0 2 153 1 0] |

- **All 312 games were drawn.** Regicide no longer loses to KingsRow at 2 s/move with the 8-piece database and the
  avoid list, but it does not win either.
- **This is not a clean A/B.** The reference differs in two things: no avoid list, and the 7-piece database. The 2 s
  "off" run with the same DLL was dropped, because neither variant won at 0.5 s, so the 2 s check was not needed
  (parent's decision).
- The run took 57 min, against 97 min for the 7-piece 2 s run. With 8 pieces many moves are instant database moves.

## Files

- Run folders, each with `summary.txt`, `results.csv`, `losses.pdn`, `runner.log`:
  - `off-kingsrow`, `off-cake`, `off-guinn`;
  - `avoid-kingsrow`, `avoid-cake`, `avoid-guinn`;
  - `bookavoid-kingsrow`, `bookavoid-cake`, `bookavoid-guinn`;
  - `avoid-kingsrow-2s`.
- `extra-65f7143/`
- `avoid_hits.txt`
- `BUILD-*.txt`, `build_769ef0b*.txt`, `book_avoid_merge.log`, `timeline.txt`
- `scripts/`
