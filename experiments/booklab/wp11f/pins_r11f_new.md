# WP11 pins from 48 games

- positions with Regicide to move (ballot end to 8 pieces): 238 distinct
- already in book book_r11f0.bin (decision or old-book move): 7 (the played move differs in 0)
- never consulted (inside a run of king moves / database-decided): 82, left out
- occurrences from lost games repaired downstream by the base book (not counted): 0; occurrences of moves the base book avoids (never pinned): 0
- positions where every move played there has a lost game through it: 0, left out; moves with a lost game dropped in positions that keep another move: 0
- **pins: 149**; positions with different moves across games: 5; of these, 2 still tie after worst result, frequency and run a, and the first move met in the frozen input order (runs sorted by name, games in file order) wins
- games without a final result skipped: 0; games rejected by the full replay: 0; games skipped because not exactly one side is Regicide against a known opponent: 0

| Ballot | Pins |
|---|---|
| ACF #6 Inferno | 149 |

**Coverage of the old line** (not r11's first own search, which only the r11 validation games show): first own non-book move in the old game mean ply 18.0 (median 18); first ply of the old game not covered by r11 with the old game's move: mean 53.0 (median 54), over 48 games. Where the pin differs from the old game's move, r11 may still answer from the book after that point, so these figures are a coverage measure; the real first search and the bank at that point are measured in the r11 validation games.

Bank estimates under the same caveat (CheckerBoard banking, 0.5 s per move; pinned moves instant):
- bl-w14c6-kingsrow-fresh round 1.1 (ACF #6 Inferno, Regicide B): first own non-book move of the old game at ply 18 with 4.0 s banked (maxtime 1.90 s); first ply of the old line not covered by r11 at ply 50, 12.0 s banked if every covered move is instant (maxtime 5.10 s)
- bl-w14c6-kingsrow-fresh round 2.1 (ACF #6 Inferno, Regicide B): first own non-book move of the old game at ply 18 with 4.0 s banked (maxtime 1.90 s); first ply of the old line not covered by r11 at ply 50, 12.0 s banked if every covered move is instant (maxtime 5.10 s)
- bl-w14c6-kingsrow-fresh round 5.1 (ACF #6 Inferno, Regicide B): first own non-book move of the old game at ply 18 with 4.0 s banked (maxtime 1.90 s); first ply of the old line not covered by r11 at ply 54, 13.0 s banked if every covered move is instant (maxtime 5.50 s)
- bl-w14c6-kingsrow-fresh round 3.1 (ACF #6 Inferno, Regicide B): first own non-book move of the old game at ply 18 with 4.0 s banked (maxtime 1.90 s); first ply of the old line not covered by r11 at ply 60, 14.5 s banked if every covered move is instant (maxtime 6.10 s)
- bl-w14c6-kingsrow-fresh round 4.1 (ACF #6 Inferno, Regicide B): first own non-book move of the old game at ply 18 with 4.0 s banked (maxtime 1.90 s); first ply of the old line not covered by r11 at ply 58, 14.0 s banked if every covered move is instant (maxtime 5.90 s)
- bl-w14c6-kingsrow-fresh round 6.1 (ACF #6 Inferno, Regicide B): first own non-book move of the old game at ply 18 with 4.0 s banked (maxtime 1.90 s); first ply of the old line not covered by r11 at ply 54, 13.0 s banked if every covered move is instant (maxtime 5.50 s)

## Positions with different moves across games (chosen move first)

- `B:WK1,10,21,23,26,27,31,32:B2,5,8,12,14,17,20,25`: **25-29** 25-29: 8 game(s) [('bl-w14c6-kingsrow-fresh', '1.1', 'D'), ('bl-w14c6-kingsrow-fresh', '2.1', 'D'), ('bl-w14c6-kingsrow-fresh', '18.1', 'D')]; 25-30: 1 game(s) [('bl-w14c6-kingsrow-kept', '2.1', 'D')]
- `B:WK1,K4,10,19,21,32:B2,5,K11,12,26`: **26-31** 26-30: 1 game(s) [('bl-w14c6-kingsrow-fresh', '5.1', 'D')]; 26-31: 6 game(s) [('bl-w14c6-kingsrow-fresh', '3.1', 'D'), ('bl-w14c6-kingsrow-fresh', '8.1', 'D'), ('bl-w14c6-kingsrow-fresh', '14.1', 'D')]
- `B:WK1,K4,14,18,19,32:B2,5,K11,12,26`: **26-31** 26-31: 5 game(s) [('bl-w14c6-kingsrow-fresh', '4.1', 'D'), ('bl-w14c6-kingsrow-fresh', '10.1', 'D'), ('bl-w14c6-kingsrow-fresh', '24.1', 'D')]; 11-7: 5 game(s) [('bl-w14c6-kingsrow-fresh', '6.1', 'D'), ('bl-w14c6-kingsrow-fresh', '15.1', 'D'), ('bl-w14c6-kingsrow-fresh', '22.1', 'D')]; 26-30: 5 game(s) [('bl-w14c6-kingsrow-fresh', '7.1', 'D'), ('bl-w14c6-kingsrow-fresh', '9.1', 'D'), ('bl-w14c6-kingsrow-fresh', '16.1', 'D')]
- `B:WK1,K4,10,15,22,27:B2,5,12,K16,K30`: **30-26** 30-26: 1 game(s) [('bl-w14c6-kingsrow-fresh', '21.1', 'D')]; 30-25: 1 game(s) [('bl-w14c6-kingsrow-kept', '6.1', 'D')]
- `B:W7,10,20,27,28:B4,5,9,12,K25`: **25-22** 25-22: 4 game(s) [('bl-w14c6-kingsrow-kept', '13.1', 'D'), ('bl-w14c6-kingsrow-kept', '12.1', 'D'), ('bl-w14c6-kingsrow-kept', '11.1', 'D')]; 9-14: 1 game(s) [('bl-w14c6-kingsrow-kept', '19.1', 'D')]

## Positions where the existing book move differs from the played move (existing kept)

