# tm-tune: time management under CheckerBoard time semantics (master b88c02e)

**Result: no time-management variant clearly beats the defaults.**
- **Self-play at 0.1 s/move:** no variant has a CI lower bound above 0, so none was re-tested at 0.3 s.
  - T5 (`tm_open_pieces=18,tm_open_factor=175`) is significantly worse: −3.7 [−7.2, −0.2].
  - T4 (`tm_soft=45`) is nearly significantly worse: −3.2 [−6.7, +0.3].
- **Against Cake:** T6 (`tm_open_pieces=20,tm_open_factor=250`), the better opening-time variant, scored −6.7 [−11.9,
  −1.4], against −11.1 [−17.8, −4.5] for the defaults on the same DLL. That is +4.4 Elo with 4 fewer losses (6 vs 10),
  well inside the CIs. Worth a longer test, but not an established gain.

## Setup

- **Code:** master **b88c02e**:
  - search parameters `tm_soft` (60), `tm_hard` (250), `tm_drop` (0 = off), `tm_drop_factor` (200), `tm_open_pieces`
    (0 = off), `tm_open_factor` (150);
  - set through CheckerBoard `enginecommand("set param name=value[,name=value]")`;
  - defaults unchanged.
- **Build checks** (`build_b88c02e.txt`):
  - bench 7,338,658 (Linux and Windows `regicide.exe`), ctest 13/13;
  - `set param` accepted through the Linux `.so` and the Windows DLL; an unknown name is rejected (`reply 0 ?`).
- **Self-play:**
  - `tools/match` from b88c02e, both sides `libregicide_cb.so` of the same build through `cbhost`. A is
    `--option-a "set param <variant>"`, B uses the defaults.
  - Time: CheckerBoard movetime mode (time banked, the `tools/match` default), **0.1 s/move**.
  - Games: `openings4.txt` 805 × 2 = 1,610, `-c 10`, hash 128, threads 1, book off, 2–8 piece EGDB.
  - Datagen was stopped during all six matches (matchlock), 23:34–00:39.
  - NNUE round-6 GPU training ran alongside until 00:17 (1 torch + 1 loader thread), affecting both sides equally.

| Variant | Parameters | W / D / L (A) | Pentanomial | Elo vs default [95% CI] |
|---|---|---|---|---|
| T1 | `tm_drop=25,tm_drop_factor=200` | 282 / 1042 / 286 | [0 36 737 32 0] | −0.9 [−4.4, +2.6] |
| T2 | `tm_drop=50,tm_drop_factor=250` | 267 / 1070 / 273 | [0 30 751 24 0] | −1.3 [−4.4, +1.8] |
| T3 | `tm_soft=80` | 272 / 1058 / 280 | [0 44 725 36 0] | −1.7 [−5.5, +2.1] |
| T4 | `tm_soft=45` | 269 / 1057 / 284 | [0 42 736 27 0] | −3.2 [−6.7, +0.3] |
| T5 | `tm_open_pieces=18,tm_open_factor=175` | 257 / 1079 / 274 | [0 43 736 26 0] | **−3.7 [−7.2, −0.2]** |
| T6 | `tm_open_pieces=20,tm_open_factor=250` | 281 / 1054 / 275 | [0 42 715 48 0] | +1.3 [−2.7, +5.3] |

- **The score-drop extension (T1, T2) and a longer or shorter soft limit (T3, T4) do not help.** The defaults (soft 60%,
  hard 250%, no drop extension) look well placed.
- **Spending more of the bank early:**
  - T6 (×2.5 with ≥ 20 pieces, i.e. only the first few moves) is neutral to slightly positive.
  - T5 (×1.75 down to 18 pieces) is worse. Taking time from the middlegame seems to cost more than the early depth
    gains.

## Against Cake

The settings match the `results/book-avoid-769ef0b` off run:
- `acf156`, 312 games, 0.5 s/move, `-c 8`;
- DLL `b88c02e-egdb8` (2–8 EGDB, no book, no avoid list), with `--option-a "set param …"` for T6;
- opponents as in the baselines;
- `tools/match` `match-747ab51`, b71dfa9 `cbhost.exe`;
- gen5c datagen (50k nodes, 12 threads) alongside.

| Run | W / D / L | Pentanomial | Elo [95% CI] | Losses as Black / White |
|---|---|---|---|---|
| off (defaults), b88c02e | 0 / 302 / 10 | [0 10 146 0 0] | −11.1 [−17.8, −4.5] | 7 / 3 |
| **T6** `tm_open_pieces=20,tm_open_factor=250` | 0 / 306 / 6 | [0 6 150 0 0] | **−6.7** [−11.9, −1.4] | 6 / 0 |

- T6 lost 4 fewer games than the defaults (6 vs 10), all as Black. The difference (+4.4 Elo) is within noise. The
  off runs against Cake today scored −12.3, −12.3, −11.1 and −10.0 (at 2 s), so −6.7 is the best Cake result so far,
  but one run of 312 games cannot separate it.
- **Average time per move** was almost the same (0.193 s vs 0.183 s), as expected: T6 only reallocates time towards the
  first moves.
- **Run times:** off 00:39–00:51, T6 00:51–01:03.

## Files

- `matches/T1-0.1s` … `matches/T6-0.1s`: `summary.txt` and `results.csv` per self-play match.
- `cake/tm-off`, `cake/tm-T6`: `summary.txt`, `results.csv`, `runner.log`.
- `build_b88c02e.txt`, `BUILD-b88c02e-egdb8.txt`, `timeline.txt`.
- `scripts/`: `tm_chain2.sh` (the whole series), `tm_build2.sh`, `run_ba2.sh`, `pausekeeper5.sh`.
