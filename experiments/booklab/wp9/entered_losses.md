# Database-lost positions entered by Regicide (WP9 step 4)

Scan of the step-4 runs with `dbscan.py --quiet --no-dedup`: a database win is counted only in a position where the side to move has no capture pending; identical games of the two builds are kept. Per side, the first won position.

| Build | Opponent | Games | Opponent had a db win | it converted | Regicide had a db win | it converted |
|---|---|---|---|---|---|---|
| conv3 | Cake | 312 | 1 | 1 | 0 | 0 |
| conv3 | GuiNN | 312 | 0 | 0 | 20 | 20 |
| conv3 | KingsRow | 312 | 1 | 1 | 11 | 11 |
| m11e | Cake | 312 | 0 | 0 | 0 | 0 |
| m11e | GuiNN | 312 | 0 | 0 | 21 | 5 |
| m11e | KingsRow | 312 | 0 | 0 | 13 | 0 |

## Every game in which the opponent had a database win against conv3

| Run | Round | Ballot | Regicide as | First lost ply | Pieces | Regicide's score on its move before | First Regicide score <= -100 (ply, pieces) | Result | m11e up to that ply |
|---|---|---|---|---|---|---|---|---|---|
| bl-s4-conv3-cake-h1 | 6.1 | ACF #7 | B | 93 | 8 | -19999 | ply 26, 16 pieces | 0-1 | deviated at ply 20 (Regicide: 16-19 vs 16-20) |
| bl-s4-conv3-kingsrow-h1 | 9.1 | ACF #10 | B | 80 | 7 | -19999 | ply 18, 20 pieces | 0-1 | deviated at ply 3 (the opponent: 24-19 vs 25-22) |
