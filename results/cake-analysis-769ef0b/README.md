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
- **Cake at 2 s/move:** see the section below (added when finished).

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

## 3. Cake at 2 s/move

*(running)*

## Files

- `cake_losses_769ef0b.pdn`: the 35 lost games.
- `report_cake_769ef0b.txt`: the full lossprobe report. `avoid_cake_769ef0b.bin`: 24 avoid entries (RGBK v2, avoid
  only).
- `blames.txt` (`blames.py`), `cake_phase.txt` / `cake_phase2.txt` (`cake_phase.py` / `cake_phase2.py`):
  the analyses above.
- `lossprobe_build.txt`, `cake_chain.sh`, `cake_2s.sh`, `timeline.txt`.
