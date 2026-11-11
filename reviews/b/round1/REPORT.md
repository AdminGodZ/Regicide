# Reviewer B, round 1

2026-11-11 08:54 UTC

## Scope and revisions

All work was isolated in /home/admingod/regicide-review-b. No match was played. Other agents' processes, locks, checkouts and job data were left alone. The existing /home/admingod/regicide-data/egdb was opened for reading only. Builds, fixtures, replays and searches ran under nice -n 19; builds used at most two jobs, searches ran sequentially on one thread, and each search stayed below 100,000,000 actual nodes. Foreground batches were bounded to less than about fifteen minutes.

| Source | Reviewed commit |
|---|---|
| origin/master | 1c6e99715299a5fbbbd407b1b9361e3c299e91e6 |
| origin/t3/truth-data | b6e49b61481df654e89003cff439da55ebaa62ea |
| origin/t3/booklab | 0d742efe9b6f27c1d044f4ae5f09f9c5e9b3a91f |
| origin/t3/review-handover | 24390f928ba2f1c05918a150b70eebd2ed0e56a3 |


The ledger on origin/master was read before review and again before reporting. Existing findings are not raised again; the two handover items are reported only as requested fix checks. No new code defect was confirmed. Initial measurements on f032e230 were put aside when master advanced; the numerical measurements below were rerun with rebuilt current master, including the PR #71 follow-up and r11d book. Old n6c/seven-piece values were read from the committed handover, not rerun.

## Search method

Release build with Ubuntu Clang 22.1.8, C++26, native CPU instructions, Ninja -j 2, LTO off. Built-in n8q SHA-256: 681549690f165eed94edafd958b06f2f76cf44ab5fba6654c82eab183efe9a24, byte-identical to truth-data's n8q file. The database reports WLD through eight pieces and conversion distances through six.

Each query uses a new Engine, one thread, 128 MiB hash cleared per query, default search parameters, book off, avoid list off, and the full record replayed into set_position(root, history). The independent time ceiling is 780 seconds; no reported query exhausted it.

Budgets are nominal 20M and 100M nodes. The engine checks nodes in blocks. The 100M request is reduced by 1,024 to respect the strict actual ceiling (typically 99,999,745 nodes). The 20M request typically finishes at 20,000,769 nodes. Raw TSVs retain all completed iterations, PVs, actual node counts and final partial-iteration status.

Scores are hundredths of a man from the side to move. Root scores below are from the last completed iteration, not an unfinished aspiration bound. Equal-depth comparisons use separate restricted-root searches, with book/table-root shortcuts disabled using infinite, each capped at 100M nodes. Reference depths come from the unrestricted 20M/100M queries; if needed the comparison uses the largest depth completed for both forced moves. These are selective search opinions, not proofs that a move loses or draws.

## Opponent positions and measurements

| Position | FEN before the move |
|---|---|
| KingsRow ACF 16 ply 41 | B:W6,15,20,30,31:B4,5,9,12,K32 |
| KingsRow ACF 43 ply 23 | B:W21,25,26,27,28,30,31:B1,4,6,7,8,14,19 |
| KingsRow ACF 71 ply 25 | B:W18,23,24,25,27,31,32:B1,5,8,10,11,12,13 |
| KingsRow ACF 134 ply 37 | B:WK4,10,13,21,22,28,30,32:B1,2,3,11,15,20,23 |
| Cake ACF 163 ply 37 | B:W9,12,15,21,25,26,32:B1,2,3,14,16,20 |
| Cake ACF 163 ply 15 | B:W13,16,23,25,26,27,29,30,31,32:B1,2,3,4,6,7,10,11,14,20 |


Unrestricted choice / completed score / completed depth:

| Position | 20M | 100M |
|---|---|---|
| KingsRow ACF 16 ply 41 | 32-28 / -41 / d32 | 32-28 / -28 / d37 |
| KingsRow ACF 43 ply 23 | 8-11 / 0 / d27 | 8-11 / 0 / d32 |
| KingsRow ACF 71 ply 25 | 5-9 / -12 / d27 | 5-9 / 0 / d32 |
| KingsRow ACF 134 ply 37 | 11-16 / -28 / d26 | 11-16 / 0 / d31 |
| Cake ACF 163 ply 37 | 14-18 / -4 / d26 | 14-18 / 0 / d31 |
| Cake ACF 163 ply 15 | 4-8 / 0 / d25 | 4-8 / 0 / d29 |


Separate equal-depth scores are played move / alternative. Gain is the alternative's score minus the played move's score.

| Position | Played/tested | Alternative | 20M-reference pair | 100M-reference pair |
|---|---|---|---|---|
| KingsRow ACF 16 ply 41 | 9-14 | 32-28 | -266 / -34 at d32; gain 232 | -251 / -25 at d36; gain 226 |
| KingsRow ACF 43 ply 23 | 8-12 | 8-11 | -90 / 0 at d27; gain 90 | -94 / 0 at d32; gain 94 |
| KingsRow ACF 71 ply 25 | 13-17 | 5-9 | -192 / -3 at d27; gain 189 | -193 / 0 at d32; gain 193 |
| KingsRow ACF 71 ply 25 | 10-14 | 5-9 | -188 / -3 at d27; gain 185 | -186 / 0 at d32; gain 186 |
| KingsRow ACF 134 ply 37 | 20-24 | 11-16 | -106 / -11 at d26; gain 95 | -126 / 0 at d31; gain 126 |
| Cake ACF 163 ply 37 | 20-24 | 14-18 | -24 / 0 at d26; gain 24 | -18 / 0 at d30; gain 18 |
| Cake ACF 163 ply 15 | 10-15 | 4-8 | -59 / 0 at d25; gain 59 | 0 / 0 at d29; gain 0 |


A gain strictly above 30 at both references is the requested rejection threshold. Match outcomes alone do not determine qualification.

### ACF 16: first qualifying KingsRow move

The two requested games, bl-k3-kingsrow-a-h1 round 14.2 and bl-v11c-n8q-kingsrow-b-h1 round 14.2, share exactly the same move/history prefix through ply 61; their first different move is White's ply 62. One independent query per shared context therefore covers both.

The first qualifying KingsRow move is **ply 41, 9-14 instead of 32-28**. The 100M unrestricted query completes depth 37, but the restricted comparison completes depth 36 for both moves. The reported pair therefore uses depth 36 rather than mixing depths.

| Ply | Played | 20M choice | Root score | Equal-depth screen | Status |
|---|---|---|---|---|---|
| 7 | 6-9 | 6-9 | -30 at d23 | same choice | not qualified at 20M |
| 9 | 7-11 | 7-11 | -54 at d23 | same choice | not qualified at 20M |
| 11 | 16-19 | 11-15 | -72 at d24 | -47 / -88 at d24; gain -41 | not qualified at 20M |
| 13 | 11-15 | 11-15 | -78 at d25 | same choice | not qualified at 20M |
| 15 | 8x15 | 8x15 | -89 at d26 | same choice | not qualified at 20M |
| 17 | 2-7 | 9-14 | -83 at d25 | -75 / -69 at d25; gain 6 | not qualified at 20M |
| 21 | 14-18 | 14-17 | -58 at d25 | -59 / -94 at d25; gain -35 | not qualified at 20M |
| 23 | 1-6 | 9-14 | -30 at d27 | -42 / -84 at d27; gain -42 | not qualified at 20M |
| 25 | 9-14 | 9-14 | -32 at d26 | same choice | not qualified at 20M |
| 27 | 6-9 | 6-9 | -32 at d27 | same choice | not qualified at 20M |
| 31 | 19-23 | 14-17 | -24 at d28 | -35 / -22 at d28; gain 13 | not qualified at 20M |
| 35 | 13-17 | 13-17 | -67 at d33 | same choice | not qualified at 20M |
| 37 | 17-22 | 17-22 | -38 at d32 | same choice | not qualified at 20M |
| 41 | 9-14 | 32-28 | -41 at d32 | -266 / -34 at d32; gain 232 | qualifies at both budgets |


All preceding KingsRow moves from ply 5 through ply 39 were replayed. Mandatory single-move roots need no preference comparison. If 20M chooses the recorded move, the two-budget rejection test cannot pass. When it chooses another move, the separate equal-depth screen is shown. The raw evidence is in K16-p*.tsv and K16-scan-log.txt.

## Replay census

The selected corpus contains 398 fully replayed, finished games: 194 mode-3 KingsRow games and 204 Cake games across ACF 16, 43, 71, 134, 163 and 165 and both colours. KingsRow's committed mode-3 runs came from k3, wp13, wp11c, wp11d, t1 and wp12. All committed matching Cake records under experiments/booklab were considered, including earlier validations and four-ply panels. Unnumbered panel tags were classified by their actual first three ballot moves. All source identities are in [corpus.tsv](corpus.tsv).

Counts match board contents, kings and side to move after legal replay, so transpositions count. The following rows are restricted to the named opening, Regicide White, and the named opponent. Every reported visit is a distinct game. These are conditional on reaching the board, not a forecast of current r11d's reach frequency. Results and points are from Regicide's side. "Losing-game move" identifies the requested move seen in opponent losses, not a game-theoretic classification.

| Position | Times reached | Losing-game move(s) | Times played | Our score in those games | Every opponent choice and our result |
|---|---|---|---|---|---|
| KingsRow ACF 16 ply 41 | 8 | 9-14 | 3 | +3 =0 -0; 3/3 points | 32-28: 5, +0 =5 -0; 2.5/5 points; 9-14: 3, +3 =0 -0; 3/3 points |
| KingsRow ACF 43 ply 23 | 6 | 8-12 | 3 | +3 =0 -0; 3/3 points | 8-11: 3, +0 =3 -0; 1.5/3 points; 8-12: 3, +3 =0 -0; 3/3 points |
| KingsRow ACF 71 ply 25 | 9 | 10-14/13-17 | 4 | +4 =0 -0; 4/4 points | 10-14: 3, +3 =0 -0; 3/3 points; 13-17: 1, +1 =0 -0; 1/1 points; 5-9: 5, +0 =5 -0; 2.5/5 points |
| KingsRow ACF 134 ply 37 | 3 | 20-24 | 3 | +3 =0 -0; 3/3 points | 20-24: 3, +3 =0 -0; 3/3 points |
| Cake ACF 163 ply 37 | 12 | 20-24 | 8 | +7 =1 -0; 7.5/8 points | 14-18: 4, +0 =4 -0; 2/4 points; 20-24: 8, +7 =1 -0; 7.5/8 points |
| Cake ACF 163 ply 15 | 32 | 10-15 | 8 | +6 =2 -0; 7/8 points | 10-15: 8, +6 =2 -0; 7/8 points; 4-8: 24, +1 =23 -0; 12.5/24 points |


Cake's 20-24 was not always followed by a loss: bl-f1-cake-a-h2 69.2 was drawn. The twelve visits include six wins in bl-w12-known163, the won bl-v13k4-cake-a-h2 69.2, that earlier drawn 20-24 game, and four 14-18 draws. The initial six-loss/three-draw subset is not the full committed census. The raw four-ply panels add two 4-8 draws to the earlier-position census. These observational outcomes have different later contexts and do not establish the causal effect of a move.

### Every opponent-position occurrence

These rows also appear with source paths and FENs in [census-occurrences.tsv](census-occurrences.tsv).

| Position | Run | Round | Ply | Move | Regicide result |
|---|---|---|---|---|---|
| K16 | bl-k3-kingsrow-a-h1 | 14.2 | 41 | 9-14 | W |
| K16 | bl-k3-kingsrow-b-h1 | 14.2 | 41 | 32-28 | D |
| K16 | bl-v11c-n8q-kingsrow-a-h1 | 14.2 | 41 | 32-28 | D |
| K16 | bl-v11c-n8q-kingsrow-b-h1 | 14.2 | 41 | 9-14 | W |
| K16 | bl-v11d-n6c-kingsrow-a-h1 | 14.2 | 41 | 32-28 | D |
| K16 | bl-v11d-n6c-kingsrow-b-h1 | 14.2 | 41 | 9-14 | W |
| K16 | bl-v11c-n6c-kingsrow-a-h1 | 14.2 | 41 | 32-28 | D |
| K16 | bl-v11c-n6c-kingsrow-b-h1 | 14.2 | 41 | 32-28 | D |
| K43 | bl-v11c-n8q-kingsrow-a-h1 | 39.2 | 23 | 8-12 | W |
| K43 | bl-v11c-n8q-kingsrow-b-h1 | 39.2 | 23 | 8-11 | D |
| K43 | bl-v11d-n6c-kingsrow-a-h1 | 39.2 | 23 | 8-11 | D |
| K43 | bl-v11d-n6c-kingsrow-b-h1 | 39.2 | 23 | 8-12 | W |
| K43 | bl-v11c-n6c-kingsrow-a-h1 | 39.2 | 23 | 8-11 | D |
| K43 | bl-v11c-n6c-kingsrow-b-h1 | 39.2 | 23 | 8-12 | W |
| K71 | bl-v11c-n6c-kingsrow-b-h1 | 63.2 | 25 | 5-9 | D |
| K71 | bl-v11c-n8q-kingsrow-a-h1 | 63.2 | 25 | 5-9 | D |
| K71 | bl-v11c-n8q-kingsrow-b-h1 | 63.2 | 25 | 5-9 | D |
| K71 | bl-k3-kingsrow-b-h1 | 63.2 | 25 | 13-17 | W |
| K71 | bl-v11d-n6c-kingsrow-b-h1 | 63.2 | 25 | 10-14 | W |
| K71 | bl-v13-kingsrow-b-h1 | 63.2 | 25 | 10-14 | W |
| K71 | bl-v13k4-kingsrow-a-h1 | 63.2 | 25 | 10-14 | W |
| K71 | bl-k3-kingsrow-a-h1 | 63.2 | 25 | 5-9 | D |
| K71 | bl-v11c-n6c-kingsrow-a-h1 | 63.2 | 25 | 5-9 | D |
| K134 | bl-v11d-n6c-kingsrow-a-h2 | 43.2 | 37 | 20-24 | W |
| K134 | bl-v11d-n6c-kingsrow-b-h2 | 43.2 | 37 | 20-24 | W |
| K134 | bl-k3-kingsrow-a-h2 | 43.2 | 37 | 20-24 | W |
| C163late | bl-w12-known163 | 13.2 | 37 | 14-18 | D |
| C163late | bl-w12-known163 | 16.2 | 37 | 14-18 | D |
| C163late | bl-w12-known163 | 17.2 | 37 | 14-18 | D |
| C163late | bl-w12-known163 | 12.2 | 37 | 20-24 | W |
| C163late | bl-w12-known163 | 11.2 | 37 | 20-24 | W |
| C163late | bl-w12-known163 | 14.2 | 37 | 20-24 | W |
| C163late | bl-w12-known163 | 9.2 | 37 | 20-24 | W |
| C163late | bl-w12-known163 | 10.2 | 37 | 20-24 | W |
| C163late | bl-w12-known163 | 15.2 | 37 | 20-24 | W |
| C163late | bl-v13k4-cake-a-h2 | 69.2 | 37 | 20-24 | W |
| C163late | bl-f1-cake-a-h2 | 69.2 | 37 | 20-24 | D |
| C163late | bl-f4-cake-h2 | 69.2 | 37 | 14-18 | D |
| C163early | bl-v11c-n8q-cake-a-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-v11c-n8q-cake-b-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-v11d-n6c-cake-a-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-v11d-n6c-cake-b-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 1.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 3.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 2.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 7.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 4.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 8.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 6.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 5.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 13.2 | 15 | 10-15 | D |
| C163early | bl-w12-known163 | 16.2 | 15 | 10-15 | D |
| C163early | bl-w12-known163 | 17.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 18.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 19.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 12.2 | 15 | 10-15 | W |
| C163early | bl-w12-known163 | 11.2 | 15 | 10-15 | W |
| C163early | bl-w12-known163 | 14.2 | 15 | 10-15 | W |
| C163early | bl-w12-known163 | 20.2 | 15 | 4-8 | D |
| C163early | bl-w12-known163 | 9.2 | 15 | 10-15 | W |
| C163early | bl-w12-known163 | 10.2 | 15 | 10-15 | W |
| C163early | bl-w12-known163 | 15.2 | 15 | 10-15 | W |
| C163early | bl-v13k4-cake-a-h2 | 69.2 | 15 | 4-8 | W |
| C163early | bl-f1-cake-a-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-f1-cake-b-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-f4-cake-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-v11c-n6c-cake-a-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-v11c-n6c-cake-b-h2 | 69.2 | 15 | 4-8 | D |
| C163early | bl-w10-b3-EB-cake-H3_1 | 43.2 | 15 | 4-8 | D |
| C163early | bl-w10-b3-O-cake-H3_1 | 43.2 | 15 | 4-8 | D |


## Seven-piece comparison

The predecessor used master 85a9fe8, n6c and seven-piece coverage. Those results alone cannot isolate the effect of the eighth piece: the network and completed depths changed too. Saved values read from its committed report/state:

| Point | Saved n6c / seven-piece result |
|---|---|
| ACF 43, 8-12 / 8-11 | -54 / -9 at d26; -75 / -3 at d31; qualified |
| ACF 71, 13-17 / 5-9 | -109 / -6 at d26; -118 / -7 at d29; qualified |
| ACF 71, 10-14 / 5-9 | -100 / -6 at d26; -101 / -10 at d31; qualified |
| ACF 134, 20-24 / 11-16 | -98 / -61 at d26; -63 / -16 at d29; qualified |
| ACF 16 ply 41, 9-14 / 32-28 | 20M chose 32-28, -38 at d29; restricted -61 / -39 at d29, gain 22; not qualified; no saved 100M pair |
| Cake ACF 163 ply 15 | 20M chose 4-8, 0 at d25; no saved equal-depth pair or 100M measurement |
| Cake ACF 163 ply 37 | 20M chose 16-19, 0 at d26; 20-24 / 16-19 = -19 / 0 at d26, gain 19; no saved 100M measurement |

To isolate coverage with the present net, I ran n8q queries with cap seven and otherwise unchanged settings. At Cake’s later ACF 163 point, cap seven chooses 16-19 at both budgets and cap eight chooses 14-18 at both budgets. The KingsRow alternative choices are unchanged; their scores and completed depths differ:

| Point | cap 7 / 20M | cap 8 / 20M | cap 7 / 100M | cap 8 / 100M |
|---|---|---|---|---|
| KingsRow ACF 16 ply 41 | 32-28 / -60 / d30 | 32-28 / -41 / d32 | 32-28 / -47 / d35 | 32-28 / -28 / d37 |
| KingsRow ACF 43 ply 23 | 8-11 / -5 / d27 | 8-11 / 0 / d27 | 8-11 / -5 / d31 | 8-11 / 0 / d32 |
| KingsRow ACF 71 ply 25 | 5-9 / -6 / d26 | 5-9 / -12 / d27 | 5-9 / -7 / d31 | 5-9 / 0 / d32 |
| KingsRow ACF 134 ply 37 | 11-16 / -42 / d26 | 11-16 / -28 / d26 | 11-16 / -5 / d31 | 11-16 / 0 / d31 |
| Cake ACF 163 ply 37 | 16-19 / -1 / d25 | 14-18 / -4 / d26 | 16-19 / 0 / d30 | 14-18 / 0 / d31 |
| Cake ACF 163 ply 15 | 4-8 / 0 / d25 | 4-8 / 0 / d25 | 4-8 / -6 / d28 | 4-8 / 0 / d29 |


Restricted controls at common completed reference depths across both moves and both coverage settings (each cell lists the lower-depth pair, then the higher-depth pair):

| Point | Moves | Common 20M/100M reference depths | n8q cap 7: low/high pairs | n8q cap 8: low/high pairs | >30 at both depths |
|---|---|---|---|---|---|
| KingsRow ACF 16 ply 41 | 9-14 / 32-28 | d32 / d34 | -65 / -64, gain 1; -80 / -46, gain 34 | -266 / -34, gain 232; -240 / -42, gain 198 | crosses only with cap 8 |
| KingsRow ACF 43 ply 23 | 8-12 / 8-11 | d27 / d31 | -115 / -21, gain 94; -105 / -3, gain 102 | -90 / 0, gain 90; -89 / 0, gain 89 | unchanged |
| KingsRow ACF 71 ply 25 | 13-17 / 5-9 | d27 / d28 | -190 / -7, gain 183; -198 / -6, gain 192 | -192 / -3, gain 189; -191 / 0, gain 191 | unchanged |
| KingsRow ACF 71 ply 25 | 10-14 / 5-9 | d27 / d31 | -186 / -7, gain 179; -185 / -5, gain 180 | -188 / -3, gain 185; -186 / -1, gain 185 | unchanged |
| KingsRow ACF 134 ply 37 | 20-24 / 11-16 | d26 / d30 | -104 / -10, gain 94; -91 / -2, gain 89 | -106 / -11, gain 95; -114 / 0, gain 114 | unchanged |
| Cake ACF 163 ply 37 | 20-24 / 14-18 | d26 / d30 | -17 / -8, gain 9; -2 / 0, gain 2 | -24 / 0, gain 24; -18 / 0, gain 18 | unchanged |
| Cake ACF 163 ply 15 | 10-15 / 4-8 | d25 / d29 | -6 / 0, gain 6; -16 / 0, gain 16 | -59 / 0, gain 59; 0 / 0, gain 0 | unchanged |


Coverage changes the paired-margin threshold in these cases: KingsRow ACF 16 ply 41 (9-14): crosses only with cap 8.

Using each coverage setting’s own unrestricted reference depths gives this separate node-budget comparison:

| Point | Moves | cap 7 own 20M/100M references | cap 8 own 20M/100M references | Two-budget result |
|---|---|---|---|---|
| KingsRow ACF 16 ply 41 | 9-14 / 32-28 | -42 / -61 at d30, gain -19; -80 / -46 at d34, gain 34 | -266 / -34 at d32, gain 232; -251 / -25 at d36, gain 226 | qualifies only with cap 8 |
| KingsRow ACF 43 ply 23 | 8-12 / 8-11 | -115 / -21 at d27, gain 94; -105 / -3 at d31, gain 102 | -90 / 0 at d27, gain 90; -94 / 0 at d32, gain 94 | same qualification |
| KingsRow ACF 71 ply 25 | 13-17 / 5-9 | -190 / -6 at d26, gain 184; -198 / -6 at d28, gain 192 | -192 / -3 at d27, gain 189; -193 / 0 at d32, gain 193 | same qualification |
| KingsRow ACF 71 ply 25 | 10-14 / 5-9 | -186 / -6 at d26, gain 180; -185 / -5 at d31, gain 180 | -188 / -3 at d27, gain 185; -186 / 0 at d32, gain 186 | same qualification |
| KingsRow ACF 134 ply 37 | 20-24 / 11-16 | -104 / -10 at d26, gain 94; -91 / -2 at d30, gain 89 | -106 / -11 at d26, gain 95; -126 / 0 at d31, gain 126 | same qualification |
| Cake ACF 163 ply 37 | 20-24 / 14-18 | -16 / -11 at d25, gain 5; -2 / 0 at d30, gain 2 | -24 / 0 at d26, gain 24; -18 / 0 at d30, gain 18 | same qualification |
| Cake ACF 163 ply 15 | 10-15 / 4-8 | -6 / 0 at d25, gain 6; -12 / 0 at d28, gain 12 | -59 / 0 at d25, gain 59; 0 / 0 at d29, gain 0 | same qualification |


At the same nominal budgets, coverage changes the classification of these named points: KingsRow ACF 16 ply 41 (9-14): qualifies only with cap 8.

This control is for the named points; I did not repeat the entire ACF 16 first-error scan with cap seven.

These remain separate selective searches, not exact values for the 10-20-piece roots. The eighth-piece table can change ordering, pruning and depth as well as leaf values. The controls state the observed changes without attributing the entire difference from n6c to the database.

## Our single-move candidates

The Black candidate board is reached by **ACF 163: 12-16 21-17 16-20 25-21**, and **ACF 165: 12-16 22-17 16-20 25-22**. Both were legally replayed and their FENs compared; the last ply is a man move and both reversible counters are zero. This is a different board from the ACF 163 line after 17-13.

The won bl-v13k4-cake-a-h2 game 69.2 plays 13-9, 11-16, 30-25 at plies 30-32. The proposal is 30-25, 11-16, 13-9. Both full prefixes were replayed and have the same board and reversible count before Cake's ply 33. The second White query uses the alternate-order board. The first forced search considers all legal Black replies, not only 11-16.

| Query | Candidate | Current book entry | FEN |
|---|---|---|---|
| Black, ACF 163/165 ply 5 | 8-12 | 11-15 | B:W17,21,22,23,24,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,20 |
| White, ACF 163 ply 30 | 30-25 | 13-9 | W:W12,13,21,26,27,30,31,32:B1,2,3,11,14,15,18,20 |
| White, alternate ACF 163 ply 32 | 13-9 | - | W:W12,13,21,25,26,27,31,32:B1,2,3,14,15,16,18,20 |
| White, ACF 43 ply 22 | 29-25 | 29-25 | W:W21,26,27,28,29,30,31:B1,4,6,7,8,14,19 |


Independent forced-candidate queries at each requested budget (move / completed score / depth), alongside the unrestricted root:

| Query | Budget | Forced candidate | Unrestricted root |
|---|---|---|---|
| Black, ACF 163/165 ply 5 | 20M | 8-12 / -1 / d24 | 11-15 / 0 / d26 |
| Black, ACF 163/165 ply 5 | 100M | 8-12 / -3 / d29 | 11-15 / 0 / d31 |
| White, ACF 163 ply 30 | 20M | 30-25 / 0 / d27 | 13-9 / 0 / d26 |
| White, ACF 163 ply 30 | 100M | 30-25 / 0 / d32 | 13-9 / 0 / d30 |
| White, alternate ACF 163 ply 32 | 20M | 13-9 / 0 / d26 | 13-9 / 2 / d26 |
| White, alternate ACF 163 ply 32 | 100M | 13-9 / 0 / d31 | 13-9 / 0 / d31 |
| White, ACF 43 ply 22 | 20M | 29-25 / 0 / d26 | 29-25 / 0 / d27 |
| White, ACF 43 ply 22 | 100M | 29-25 / 0 / d32 | 29-25 / 0 / d32 |


Equal-depth candidate/reference comparisons at the same nominal budget; a candidate penalty strictly above 15 is the requested reason against it:

| Query | Budget | Candidate / reference | Equal-depth scores | Candidate penalty | Threshold result |
|---|---|---|---|---|---|
| Black, ACF 163/165 ply 5 | 20M | 8-12 / 11-15 | -1 / 0 at d24 | 1 | no >15 reason against |
| Black, ACF 163/165 ply 5 | 100M | 8-12 / 11-15 | -3 / 0 at d29 | 3 | no >15 reason against |
| White, ACF 163 ply 30 | 20M | 30-25 / 13-9 | 0 / 4 at d25 | 4 | no >15 reason against |
| White, ACF 163 ply 30 | 100M | 30-25 / 13-9 | 0 / 0 at d30 | 0 | no >15 reason against |
| White, alternate ACF 163 ply 32 | 20M | 13-9 / 13-9 | same move | 0 | no >15 penalty against this move |
| White, alternate ACF 163 ply 32 | 100M | 13-9 / 13-9 | same move | 0 | no >15 penalty against this move |
| White, ACF 43 ply 22 | 20M | 29-25 / 29-25 | same move | 0 | no >15 penalty against this move |
| White, ACF 43 ply 22 | 100M | 29-25 / 29-25 | same move | 0 | no >15 penalty against this move |


At the alternate White ply-32 board there is no book entry; the reference is the proposed 13-9, also the unrestricted choice at both budgets. At ACF 43 ply 22, current r11d already plays 29-25, so it is no longer a proposed change relative to that book. Absence of a >15 penalty is not a proof or a match-based acceptance of a book edit.

## Code review and fixtures

### truth relabel / labelcheck

Read tools/relabel.hpp, tools/truth.cpp (database conversion, relabel, labelcheck, Reader and for_each_game), src/datafile.hpp, the added relabel regression, and the database API on the fetched truth-data branch.

The helper now probes every covered record separately. color() masks the STM bit; db_black() converts the database's side-to-move value to Black's result exactly once. Search scores and flags are preserved. Records before the first known covered position receive its Black result; later Unknown records carry the nearest earlier known Black result; a wholly uncovered game is unchanged. That Unknown fallback is a carried game label, not an exact database value. labelcheck counts Unknown separately and does not call it a wrong known result.

Executed the actual branch truth CLI with the real eight-piece database. Its eight-record fixture includes a nine-piece prefix, Black/White-to-move three-piece records, a later two-piece draw, separate game starts, and both colours at eight pieces. All bytes except result bytes are preserved. Output Black results: 1,1,1,0,-1,0,-1,1. Afterward labelcheck reports zero wrong values among seven covered records, including both eight-piece records.

Executed same-material flagged boundaries and an unflagged legacy prefix followed by a flag. Both report two games and give Black results 0,1. A material increase was not needed to split them. After the first GAME_START, boundaries use flags; the legacy prefix uses the existing heuristic. This does not certify exact boundaries for wholly legacy files.

Executed a separate helper fixture with a stubbed probe returning Unknown on legal record shapes: it checks carrying across unknown records, updating on a later known value, both result perspectives, and leaving an entirely unknown game unchanged (three own values, two carried). No database slice was removed or modified. Unknown handling in the actual CLI was read; no real missing-slice case was manufactured in the complete shared database.

Logs: relabel-fixture.txt, relabel-unknown.txt, relabel-cli.txt, labelcheck-before.txt, labelcheck-after.txt, fixture-bytecheck.txt, boundary-flagged.txt and boundary-mixed.txt. No new defect confirmed.

### Engine::go, PR #65 and its PR #71 follow-up

Read the root database probe, egdb_root_move, draws_at_once, and set_position's history reconstruction. The child legal-move check precedes draw adjudication; repetition checks only the reversible-history window, and side to move is part of the key. A lost root takes a third occurrence; a won root rejects a table move that immediately draws. The current follow-up also handles a restricted root-move list.

Executed independent legal four-ply king cycles from the real eight-piece database. For each of 3, 4, 6 and 8 pieces and both sides to move, begin at a cycle child and replay seven legal reversible plies. The selected next child has occurred twice, without an earlier third occurrence. Every root was verified database-Loss.

Each case was tested with requested MTC cap 0 and 8 (available distances end at six): normal result draw/zero nodes, restricted repeating move draw/zero nodes, and the same board without history retaining a negative score. All **48 assertions pass**. These fixtures use HCE, one thread, hash 128 MiB, depth 2, and a 10,000-node cap.

| Pieces | Lost side | Root | Immediate repetition move |
|---|---|---|---|
| 3 | Black | B:WK7,K24:BK18 | 18-14 |
| 3 | White | W:WK10:BK11,K29 | 10-6 |
| 4 | Black | B:WK2,K3,K27:BK1 | 1-5 |
| 4 | White | W:WK8:BK10,K14,K22 | 8-3 |
| 6 | Black | B:WK10,K15,K19,K32:BK1,K13 | 13-9 |
| 6 | White | W:WK9,K32:BK7,K10,K13,K23 | 9-5 |
| 8 | Black | B:WK9,K12,K14,K21,K28:BK5,K25,K26 | 25-29 |
| 8 | White | W:WK7,K31,K32:BK8,K17,K18,K22,K26 | 32-27 |


The whole ctest suite, sanitizers and matches were not run in this round. Targeted fixtures are not an exhaustive certification of all eight-piece roots.

## Handover fix checks

Both requested cases are fixed at the reviewed current Book Lab head.

- pins.py: the saved king-run fixture counts one downstream repaired occurrence and emits the expected one pin (the earlier head emitted zero). Its avoid-list test no longer requires book eligibility. The saved public Engine control was rebuilt with a 10,000-node cap: at reversible count 1 it avoids 5-9 and selects 1-6 in 11 nodes.
- wins_vs_book.py: the saved valid FEN-started game walks all 16 own positions without replay errors. An invalid final 1-2 produces REPLAY FAILED and exit 3 with no affirmative row. The current report separates no forced departure from the first book miss; the valid fixture's first miss is ply 2.

These are requested checks of ledgered findings, not new findings. See pins-king-fixture.md, king-policy.txt, wins-fixture.txt and wins-invalid-fixture.txt.

## Commands and evidence

Foreground commands ran through wsl.exe -u admingod -- bash. Snapshots were extracted using git archive from the fetched commits above. Only this round's report text and evidence is committed to t3/review-reports; no engine source file or binary is committed.

```sh
nice -n 19 git clone --no-single-branch https://github.com/AdminGodZ/Regicide.git ~/regicide-review-b
nice -n 19 git fetch origin
nice -n 19 cmake -S snapshots/master -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++-22 -DREGICIDE_LTO=OFF
nice -n 19 cmake --build build -j 2 --target regicide regicide_core regicide_egdb egdb_root_test
nice -n 19 clang++-22 -std=c++26 -O3 -march=native -I snapshots/master/src -I snapshots/booklab/tools/booklab work/review.cpp build/libregicide_core.a build/libregicide_egdb.a -pthread -o work/review
# Query: game ID, one-based ply, budget, target depth, forced move or -, database cap.
nice -n 19 work/review query work/games.tsv G99 23 20000000 127 - 8
nice -n 19 work/review query work/games.tsv G99 23 100000000 32 8-12 8
nice -n 19 work/review walk work/games.tsv
nice -n 19 work/truth relabel --egdb /home/admingod/regicide-data/egdb --in work/fixture-input.bin --out work/fixture-output.bin
nice -n 19 work/truth labelcheck --egdb /home/admingod/regicide-data/egdb work/fixture-output.bin
nice -n 19 work/root_fixture-current
```

| Query | ID / ply | Source record / round |
|---|---|---|
| KingsRow ACF 16 ply 41 | G1 / 41 | k3/games/bl-k3-kingsrow-a-h1.pdn.gz / 14.2 |
| KingsRow ACF 43 ply 23 | G99 / 23 | wp11c/games/bl-v11c-n6c-kingsrow-b-h1.pdn.gz / 39.2 |
| KingsRow ACF 71 ply 25 | G16 / 25 | k3/games/bl-k3-kingsrow-b-h1.pdn.gz / 63.2 |
| KingsRow ACF 134 ply 37 | G7 / 37 | k3/games/bl-k3-kingsrow-a-h2.pdn.gz / 43.2 |
| Cake ACF 163 ply 37 | G273 / 37 | wp13/games/bl-v13k4-cake-a-h2.pdn.gz / 69.2 |
| Cake ACF 163 ply 15 | G273 / 15 | wp13/games/bl-v13k4-cake-a-h2.pdn.gz / 69.2 |
| Black, ACF 163/165 ply 5 | G32 / 5 | wp10/games/bl-f1-cake-a-h2.pdn.gz / 69.1 |
| White, ACF 163 ply 30 | G273 / 30 | wp13/games/bl-v13k4-cake-a-h2.pdn.gz / 69.2 |
| White, alternate ACF 163 ply 32 | GALT / 32 | alternate prefix of wp13/games/bl-v13k4-cake-a-h2.pdn.gz / 69.2 |
| White, ACF 43 ply 22 | G99 / 22 | wp11c/games/bl-v11c-n6c-kingsrow-b-h1.pdn.gz / 39.2 |


Raw completed-iteration traces and fixture outputs accompany the report as UTF-8 text. The review-only drivers and fixture sources are reproduced as report text in the appendix; their executable files remain locally in the isolated review directory. Saved predecessor measurements were read from experiments/review-handover/b14/REPORT.md and wins/state.json on origin/t3/review-handover.

## Review-only reproduction sources

These are the query/fixture harnesses used for the measurements above, reproduced as report text rather than changes to engine source. Query identities and committed records are listed above; the TSV loader format is id, starting FEN (empty for the normal start), our colour, ballot plies, and full space-separated main-line moves. The GALT record replaces White plies 30/32 by the proposed order and otherwise follows G273.

### review.cpp

```cpp
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include "search.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
#include "movetext.hpp"
using namespace regicide;
std::vector<std::string> split(const std::string&s,char c='\t'){std::vector<std::string>v;std::stringstream ss(s);for(std::string x;std::getline(ss,x,c);)v.push_back(x);return v;}
struct Game{std::string id,fen,col;int nopen;std::vector<std::string>mv;};
int main(int ac,char**av){try{
if(ac<3)return 2; std::ifstream in(av[2]);std::map<std::string,Game>gs;
for(std::string l;std::getline(in,l);){auto f=split(l);Game g{f[0],f[1],f[2],std::stoi(f[3]),{}};std::istringstream ms(f[4]);for(std::string m;ms>>m;)g.mv.push_back(m);gs[g.id]=g;}
std::string mode=av[1];
if(mode=="walk"){
auto book=Book::builtin();std::cout<<"game\tply\tfen\trev\tplayed\tbook\tlegal\n";
for(auto&[id,g]:gs){Position p;if(!g.fen.empty()&&!p.set_fen(g.fen))throw std::runtime_error("fen");int ply=0;
for(auto&t:g.mv){Move m;if(!parse_move(p,t,m)||booklab::matching_moves(p,t)!=1)throw std::runtime_error("replay "+id+" "+std::to_string(ply+1));auto bm=book->probe(p,0,0,book.get());MoveList ml;generate_moves(p,ml);std::cout<<id<<'\t'<<++ply<<'\t'<<p.fen()<<'\t'<<p.reversible_plies()<<'\t'<<move_text(p,m)<<'\t'<<(bm?move_text(p,*bm):"-")<<'\t'<<ml.size()<<'\n';p.make_move(m);}}
return 0;}
if(ac!=9)return 2;auto&g=gs.at(av[3]);int ply=std::stoi(av[4]);uint64_t budget=std::stoull(av[5]);int depth=std::stoi(av[6]);std::string force=av[7];int cap=std::stoi(av[8]);if(budget>100000000)throw std::runtime_error("budget");
if(!egdb::init("/home/admingod/regicide-data/egdb",64))throw std::runtime_error("DB");
Position p;if(!g.fen.empty()&&!p.set_fen(g.fen))throw std::runtime_error("fen");std::vector<Position>hist;
for(int i=0;i<ply-1;++i){Move m;if(!parse_move(p,g.mv.at(i),m))throw std::runtime_error("move");hist.push_back(p);p.make_move(m);}
Engine e;e.set_threads(1);e.set_hash_mb(128);e.set_egdb_pieces(cap);e.set_book(nullptr);e.set_avoid(nullptr);e.new_game();e.set_position(p,hist);
Limits l;l.nodes=budget==100000000?budget-1024:budget;l.depth=depth;l.movetime_ms=780000;l.use_book=false;
if(force!="-"){Move m;if(!parse_move(p,force,m))throw std::runtime_error("force "+force);l.searchmoves={m};l.infinite=true;}
SearchInfo c;
std::cout<<"# root "<<p.fen()<<" rev "<<p.reversible_plies()<<" dbcap "<<cap<<"\n";
std::cout<<"depth\tscore\tmove\tnodes\ttbhits\tpv\n";
auto r=e.go(l,[&](const SearchInfo&i){c=i;std::cout<<i.depth<<'\t'<<i.score<<'\t'<<(i.pv.empty()?"none":move_text(p,i.pv[0]))<<'\t'<<i.nodes<<'\t'<<i.tbhits<<'\t';Position q=p;for(auto m:i.pv){std::cout<<move_text(q,m)<<' ';q.make_move(m);}std::cout<<std::endl;});
std::cout<<"# final depth "<<r.depth<<" completed "<<r.completed_score<<" score "<<r.score<<" move "<<move_text(p,r.best)<<" partial "<<r.partial<<" nodes "<<r.nodes<<" ms "<<r.time_ms<<"\n";
return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 9;}}
```

### scan16.py

```python
import json,csv,subprocess,time
from pathlib import Path
g=next(x for x in json.load(open('work/games.json')) if x['id']=='G1')
walk={int(r['ply']):r for r in csv.DictReader(open('work/walk.tsv'),delimiter='\t') if r['game']=='G1'}
start=time.monotonic();done=False;summary=[]
def query(ply,budget,move='-',depth=127):
 tok=move.replace('x','_')
 p=Path(f'reports/round1/K16-p{ply}-{budget}-{tok}-d{depth}.tsv')
 if not p.exists():
  with p.open('w') as f:subprocess.run(['nice','-n','19','work/review','query','work/games.tsv','G1',str(ply),str(budget),str(depth),move,'8'],stdout=f,check=True)
 lines=[x for x in p.read_text().splitlines() if not x.startswith('#')]
 rows=list(csv.DictReader(lines,delimiter='\t'));return rows
def pair(ply,played,alt,depth):
 a=query(ply,100000000,played,depth);b=query(ply,100000000,alt,depth)
 common=min(int(a[-1]['depth']),int(b[-1]['depth']),depth)
 x=next(r for r in a if int(r['depth'])==common);y=next(r for r in b if int(r['depth'])==common)
 return dict(depth=common,played=int(x['score']),alternative=int(y['score']),gain=int(y['score'])-int(x['score']))
for ply in range(5,min(len(g['moves'])+1,62),2):
 r=walk[ply]
 if int(r['legal'])<2:continue
 a=query(ply,20000000)[-1];played=g['moves'][ply-1]
 row=dict(ply=ply,played=played,depth20=int(a['depth']),choice20=a['move'],score20=int(a['score']))
 if a['move']!=played:
  p=pair(ply,played,a['move'],int(a['depth']));row['pair20']=p
  if p['gain']>30:
   b=query(ply,100000000)[-1];row.update(choice100=b['move'],score100=int(b['score']),depth100=int(b['depth']))
   if b['move']!=played:
    q=pair(ply,played,b['move'],int(b['depth']));row['pair100']=q
    if b['move']!=a['move']:row['pair20']=pair(ply,played,b['move'],int(a['depth']))
    if q['gain']>30 and row['pair20']['gain']>30:
     done=True;row['QUALIFIED']=True
 summary.append(row);print(row,flush=True)
 if done or time.monotonic()-start>540:break
Path('work/scan16.json').write_text(json.dumps(dict(done=done,summary=summary),indent=2))
```

### root_fixture.cpp

```cpp
#include <iostream>
#include <random>
#include "search.hpp"
#include "movegen.hpp"
#include "egdb/egdb.hpp"
using namespace regicide;using egdb::Wld;
Wld val(const Position&p){return egdb::probe(p.pieces(BLACK),p.pieces(WHITE),p.kings(),p.side_to_move());}
int main(){if(!egdb::init("/home/admingod/regicide-data/egdb",64))return 2;std::mt19937 rng(918273);int bad=0;
for(int pieces:{3,4,6,8})for(Color side:{BLACK,WHITE}){
 Position root;std::vector<Move>cycle;bool found=false;
 for(int trial=0;trial<10000&&!found;++trial){Bitboard b=0,w=0;int small=pieces==3?1:pieces/2-1;
 for(int i=0;i<pieces;++i){int sq;do{sq=rng()%32;}while((b|w)&(1u<<sq));if((i<small)==(side==BLACK))b|=1u<<sq;else w|=1u<<sq;}
 Position p;p.set(b,w,b|w,side);if(val(p)!=Wld::Loss)continue;MoveList a;generate_moves(p,a);
 for(auto m:a){if(m.captures)continue;auto q=p;q.make_move(m);MoveList bb;generate_moves(q,bb);
 for(auto n:bb){if(n.captures)continue;auto r=q;r.make_move(n);MoveList cc;generate_moves(r,cc);
 for(auto o:cc){if(o.captures)continue;auto t=r;t.make_move(o);MoveList dd;generate_moves(t,dd);
 for(auto z:dd){if(z.captures)continue;auto u=t;u.make_move(z);if(u==p){root=p;cycle={m,n,o,z};found=true;break;}}if(found)break;}if(found)break;}if(found)break;}}
 if(!found){std::cout<<"FAIL no cycle "<<pieces<<' '<<int(side)<<"\n";++bad;continue;}
 // Begin at child, then replay seven legal reversible plies: child occurs twice before this root.
 Position child=root;child.make_move(cycle[0]);Position p=child;p.set_reversible_plies(0);std::vector<Position> hist;
 for(int i=0;i<7;++i){hist.push_back(p);p.make_move(cycle[(i+1)%4]);}
 if(!(p==root)||p.reversible_plies()!=7)return 4;
 for(int mtc:{0,8}){Engine e;e.set_threads(1);e.set_hash_mb(128);e.set_egdb_pieces(8);e.set_egdb_mtc_pieces(mtc);e.set_book(nullptr);e.set_avoid(nullptr);e.set_evaluator(Evaluator::hce());e.set_position(p,hist);Limits l;l.depth=2;l.nodes=10000;auto rr=e.go(l);Position c=p;c.make_move(rr.best);int count=0;for(auto &h:hist)count+=h==c;bool ok=rr.score==0&&count>=2;bad+=!ok;std::cout<<(ok?"PASS":"FAIL")<<" repeat pieces "<<pieces<<" side "<<int(side)<<" mtc "<<mtc<<" root "<<p.fen()<<" move "<<move_text(p,rr.best)<<" score "<<rr.score<<" nodes "<<rr.nodes<<" child_occurrences "<<count<<"\n";
 e.new_game();e.set_position(p,hist);l.searchmoves={rr.best};auto restricted=e.go(l);bool rok=restricted.best==rr.best&&restricted.score==0&&restricted.nodes==0;bad+=!rok;std::cout<<(rok?"PASS":"FAIL")<<" restricted repeat score "<<restricted.score<<" nodes "<<restricted.nodes<<"\n";l.searchmoves.clear();
 // No history, no immediate draw by repetition: root remains database-lost.
 e.new_game();e.set_position(p);auto ctrl=e.go(l);bool lost=ctrl.score<0;bad+=!lost;std::cout<<(lost?"PASS":"FAIL")<<" no-history score "<<ctrl.score<<"\n";

 }}
return bad?1:0;}
```

### relabel_fixture.cpp

```cpp
#include <fstream>
#include <iostream>
#include <vector>
#include "relabel.hpp"
using namespace regicide;
DataRecord rec(const char*s,int r,bool flag=false){Position p;if(!p.set_fen(s))throw 1;auto d=DataRecord::make_proven(p,3000);d.result=r;if(flag)d.stm|=DataRecord::GAME_START;return d;}
int main(){if(!egdb::init("/home/admingod/regicide-data/egdb",64))return 2;
std::cout<<"db "<<egdb::max_pieces()<<" mtc "<<egdb::max_mtc_pieces()<<"\n";
std::vector<DataRecord> a{rec("W:WK13,K30,K31,K32,K28:BK14,K22,K1,K2",0,true),rec("B:WK13:BK14,K22",0),rec("W:WK13:BK14,K22",0),rec("B:WK22:BK14",1)};
auto originals=a;auto c=truth::relabel_game(a,8);int bad=0;
for(size_t i=0;i<a.size();++i){int v=i?truth::db_black(a[i]):truth::db_black(a[1]);bool ok=a[i].result==v&&a[i].stm==originals[i].stm&&a[i].score==originals[i].score;bad+=!ok;std::cout<<(ok?"PASS":"FAIL")<<" record "<<i<<" stm "<<int(a[i].stm)<<" own "<<truth::db_black(a[i])<<" result "<<int(a[i].result)<<"\n";}
std::vector<DataRecord>b{rec("W:WK22,K23:BK14",0,true),rec("B:WK22:BK14",1)};
auto all=originals;all.insert(all.end(),b.begin(),b.end());
for(const char* f:{"B:WK9,K12,K14,K21,K28:BK5,K25,K26","W:WK7,K31,K32:BK8,K17,K18,K22,K26"}){auto x=rec(f,0,true);std::vector<DataRecord> v{x};auto z=truth::relabel_game(v,8);bool ok=z.own==1&&v[0].result==truth::db_black(x);bad+=!ok;std::cout<<(ok?"PASS":"FAIL")<<" 8-piece "<<f<<" Black-result "<<int(v[0].result)<<"\n";all.push_back(x);}
std::ofstream out("work/fixture-input.bin",std::ios::binary);out.write((char*)all.data(),all.size()*sizeof(DataRecord));
std::cout<<"changed "<<c.changed<<" own "<<c.own<<" carried "<<c.carried<<"\n";
return bad?1:0;}
```

### relabel_unknown.cpp

```cpp
#include <iostream>
#include <map>
#include <tuple>
#include "relabel.hpp"
using namespace regicide;
using K=std::tuple<Bitboard,Bitboard,Bitboard,int>;
std::map<K,egdb::Wld> vals;
namespace regicide::egdb {Wld probe(Bitboard b,Bitboard w,Bitboard k,Color s){auto i=vals.find({b,w,k,int(s)});return i==vals.end()?Wld::Unknown:i->second;}}
DataRecord r(unsigned n,Color side,egdb::Wld v){DataRecord x;x.black=1u<<n;x.white=1u<<(n+8);x.kings=x.black|x.white;x.stm=side|DataRecord::GAME_START|DataRecord::PROVEN;x.result=-1;vals[{x.black,x.white,x.kings,int(side)}]=v;return x;}
int main(){auto a=r(0,BLACK,egdb::Wld::Win),u=r(1,WHITE,egdb::Wld::Unknown),b=r(2,WHITE,egdb::Wld::Win),c=r(3,BLACK,egdb::Wld::Loss);DataRecord pre=a;pre.black=0x1ff;pre.white=0x100000;pre.kings=pre.black|pre.white;std::vector<DataRecord>g{pre,a,u,b,u,c};auto q=truth::relabel_game(g,8);int want[]={1,1,1,-1,-1,-1};int bad=0;for(int i=0;i<6;++i){bool ok=g[i].result==want[i];bad+=!ok;std::cout<<(ok?"PASS":"FAIL")<<" stub-record "<<i<<" result "<<int(g[i].result)<<"\n";}std::vector<DataRecord>h{u};auto z=truth::relabel_game(h,8);bool ok=z.changed==0&&!z.covered&&h[0].result==-1;bad+=!ok;std::cout<<(ok?"PASS":"FAIL")<<" wholly unknown unchanged\n";std::cout<<"own "<<q.own<<" carried "<<q.carried<<"\n";return bad;}
```
