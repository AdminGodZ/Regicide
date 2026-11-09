# Cake analysis (master 769ef0b, 8-piece EGDB)

Cake 1.89f is the only opponent Regicide still clearly loses to: about −12 Elo at 0.5 s/move (`results/book-avoid-769ef0b`:
off −12.3, avoid −12.3, book+avoid −14.5).

**Summary**
- **Losses come from the opening, as Black.** Of the 35 Cake losses in today's three 769ef0b runs, 34 are with
  Regicide as Black.
- **lossprobe blames a move in the first 10 moves after the 3-move ballot in 25 of 32 cases** (21 within 5 moves).
  The median blamed move is the 5th after the ballot, with 18–24 pieces still on the board. 6 blames fall in the
  middlegame and 1 in the endgame.
- **The same ballots lose again and again.** 10 of 22 ballots lost in two or all three runs. Wilderness II, Gemini I
  and ACF #63 lost every time. 8 of the 24 distinct blamed (position, move) pairs were repeated in different runs.
- **The losses unfold slowly.** Regicide's own score first stays at or below −100 at a median of move 15 after the
  ballot. Cake's eval stays below +50 until about 9 pieces, when its database view resolves the game.
- **The existing avoid list does not cover these mistakes.** Only 2 of the 24 new blamed pairs are in avoid_v1, and
  avoid_v1 had no effect on the Cake score. Cake keeps finding new early mistakes, mostly in ballots that are hard
  for Black.
- **Cake at 2 s/move: −10.0 [−16.4, −3.7]** (0/303/9) vs −12.3 at 0.5 s. The gap hardly shrinks with 4× the time,
  unlike KingsRow.

## 1. The losses

These are the 769ef0b runs vs Cake from `results/book-avoid-769ef0b`: 312 games each, 0.5 s/move, 8-piece EGDB.

| Run | Losses | Elo |
|---|---|---|
| off | 11 | −12.3 |
| avoid | 11 | −12.3 |
| book+avoid | 13 | −14.5 |

That is 35 lost games (`cake_losses_769ef0b.pdn`), with no wins in any run.

- **Colour:** 34 as Black, 1 as White (Black Widow, book+avoid).
- **Ballots** (22 distinct; `cake_phase2.txt`, `blames.txt`):
  - lost in all three runs: ACF #16 Wilderness II, ACF #62 Gemini I, ACF #63;
  - lost in two runs: ACF #5 Dreaded Edinburgh, #6 Inferno, #7, #9 Wilderness I, #14, #74, #141.
- **When the game turned** (`cake_phase.py`, `cake_phase2.py`, from the engines' PDN comments):
  - Regicide's own score first ≤ −100 and never back above −50: median move 15 after the ballot (quartiles 13–18).
    By then 10–16 pieces are left: opening 2, middlegame 11, endgame 22.
  - Cake's own eval first ≥ +50 and never below +25: median move 36. Cake's scale is about half of Regicide's, and
    its eval stays small until its database resolves the game, so it is not a useful marker for when the game was
    lost.

## 2. lossprobe

```
lossprobe --pdn cake_losses_769ef0b.pdn --movetime 10000 --threads 6 --jobs 2 --hash 1024 \
  --egdb ~/regicide-data/egdb --out avoid_cake_769ef0b.bin --report report_cake_769ef0b.txt
```

- **Setup:**
  - lossprobe from master 769ef0b (Linux, clang, `-march=native`; bench 7,338,658; `lossprobe_build.txt`);
  - 2–8 piece EGDB;
  - default rule: drop 30, margin 20, lost 300; ballot from the Opening header.
  - Ran 22:04–22:36 (31 min) with gen5a datagen alongside and no matches.
- **Result:** 35 losses analysed, **32 blamed, 3 unclear** (ACF #19, #24, #5 Dreaded Edinburgh; all book+avoid),
  0 errors. `avoid_cake_769ef0b.bin` holds **24 distinct avoid entries**.

| Blamed move, moves after the ballot | Games |
|---|---|
| 1–5 (including move 0, the first move after the ballot) | **21** |
| 6–10 | **4** |
| 11–20 | 6 |
| 21–30 | 1 |

- **Phase:** opening (first 10 moves after the ballot) **25**, middlegame (> 12 pieces) **6**, endgame (≤ 12 pieces)
  **1**.
- **Pieces at the blamed move:** 24: 3, 22: 7, 20: 10, 19: 4, 18: 3, 16: 3, 14: 1, 9: 1.
- **Size of the mistakes:** the played move scored 34–194 cp below the best alternative at 10 s; the median drop is
  about 75.
- **No blame is a move into a database loss.**
- **Repeated mistakes:** 8 of the 24 blamed (position, move) pairs occur in two runs:

| Move | Ballot | Move after ballot |
|---|---|---|
| 15-18 | ACF #7 | 5 |
| 14-17 | Wilderness I | 11 |
| 7-11 | Wilderness II | 1 |
| 5-9 | ACF #14 | 0 |
| 10-14 | Gemini I | 4 |
| 8-11 | ACF #74 | 5 |
| 1-5 | ACF #63 | 4 |
| 7-11 | ACF #37 | 12 |

- **Overlap with avoid_v1:** 2 of the 24 pairs are in avoid_v1, both from the off run; the avoid run cannot play
  them. In the avoid and book+avoid runs, Regicide lost the same ballots through other early moves.

**Reading:** the Cake gap is an opening problem for Black in a set of hard ballots. The mistakes are small (about
0.5–1 man at 10 s) and come early, and Cake converts them slowly. A per-move avoid list chases them one at a time. A
stronger fix would be a deeper opening book for these ballots, analysed at much more than 1M nodes per move, or more
search in the first 10 moves after the ballot.

## 3. Cake at 2 s/move, and a 0.5 s repeat

Both runs use the same DLL as the 0.5 s runs: 769ef0b, 2–8 EGDB, no book or avoid list. Settings are acf156, 312
games and `-c 8` (`off-769-2s-cake/`, `off-769b-cake/`).

| Run | W / D / L | Elo [95% CI] | Pentanomial | Losses as Black |
|---|---|---|---|---|
| 0.5 s off (`results/book-avoid-769ef0b/off-cake`) | 0 / 301 / 11 | −12.3 [−19.2, −5.3] | [0 11 145 0 0] | 10 of 11 |
| **0.5 s off, repeat** | 0 / 301 / 11 | **−12.3** [−19.2, −5.3] | [0 11 145 0 0] | 9 of 11 |
| **2 s off** | 0 / 303 / 9 | **−10.0** [−16.4, −3.7] | [0 9 147 0 0] | 8 of 9 |

- **The Cake gap hardly shrinks with 4× the time.** It goes from −12.3 to −10.0, both CIs well below 0. Against
  KingsRow, the same step went from −4.5 to −1.1, and to 0.0 with the 8-piece database and avoid list.
- This fits section 2. **The losing mistakes are early and small**, so 4× the search time does not find them. Cake's
  edge here looks like opening knowledge, through its own book and evaluation, not search depth.
- **8 of the 9 ballots lost at 2 s were also lost at 0.5 s:** Inferno, ACF #31, Gemini I, White Doctor, #79, #90,
  #143, #154 as Black, and Black Widow as White. The repeat at 0.5 s lost Inferno, Edinburgh Single, Wilderness I,
  #14, Wilderness II, Gemini I, White Doctor, #85 and Tyne as Black, and Double Cross and #129 as White.
- **Run times:** 2 s 22:36–23:22 (46 min; many moves are instant database moves); repeat 23:22–23:34.
- **Background:**
  - 2 s run: gen5a datagen (50k nodes, 12 threads) until it finished at 22:40, then nothing.
  - Repeat: nothing for its first minute, then gen5c (the same 50k-node datagen, seed 62) from 23:23. The first 0.5 s
    run had bookgen in the background, so two different backgrounds gave the identical −12.3.

**Next** (per the plan): a deep opening book targeted at exactly these ballots (`t3/book-deep`), and an
opening-time-management test against Cake (`t3/tm-tune`).

## Files

- `cake_losses_769ef0b.pdn`: the 35 lost games.
- `report_cake_769ef0b.txt`: the full lossprobe report. `avoid_cake_769ef0b.bin`: 24 avoid entries (RGBK v2, avoid
  only).
- `blames.txt` (`blames.py`), `cake_phase.txt` / `cake_phase2.txt` (`cake_phase.py` / `cake_phase2.py`):
  the analyses above.
- `lossprobe_build.txt`, `cake_chain.sh`, `cake_2s.sh`, `timeline.txt`.
