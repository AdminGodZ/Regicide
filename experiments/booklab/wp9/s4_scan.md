# WP8: missed database wins

Games: every games.pdn under `/home/admingod/regicide-jobs/booklab/s4_scan/runs` (Regicide vs Cake/KingsRow/GuiNN, all engine versions, plus the games among the three opponents), 0 duplicate games removed, 0 games without a final result ("*" or none: interrupted or unfinished) skipped, 0 games rejected (a move that does not parse to exactly one legal move), 0 duplicate records with the same moves but another result or termination (the first record is kept; listed at the end). Database: Regicide's 2–8-piece WLD.
A game *reached a database win* when some position with <= 8 pieces is a database win for one side.

| Games | Scanned | Reached a database win | Winning side won | Not won |
|---|---|---|---|---|
| Regicide vs Cake/KingsRow/GuiNN | 1872 | 67 | 38 | 29 |
| Opponent vs opponent | 0 | 0 | 0 | 0 |

Conversion by the side that had the database win: Regicide 36/65; Cake 1/1; KingsRow 1/1

## 1. Regicide had a database win and did not win (29)

| Run | Round | Ballot | Winner side | Against | First won ply | Pieces | Result | Termination | How it was lost | FEN of the first won position |
|---|---|---|---|---|---|---|---|---|---|---|
| bl-s4-m11e-guinn-h1 | 20.1 | ACF #22 | Regicide as Black | GuiNN | 75 | 8 | D | threefold repetition | winner moved into a non-won position at ply 142 (17-14, 1 such move); final db value D | `W:WK6,9,21,26:B13,16,K18,K19` |
| bl-s4-m11e-guinn-h1 | 21.2 | ACF #23 | Regicide as White | GuiNN | 80 | 8 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `B:WK16,20,21,22,K27:B12,13,K28` |
| bl-s4-m11e-guinn-h1 | 22.2 | ACF #24 | Regicide as White | GuiNN | 99 | 8 | D | threefold repetition | winner moved into a non-won position at ply 105 (10-6, 1 such move); final db value D | `W:WK2,10,K17,32:BK9,28,K29,K31` |
| bl-s4-m11e-guinn-h1 | 23.2 | ACF #25 | Regicide as White | GuiNN | 72 | 8 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `B:WK17,K20,21,25:B18,19,26,K28` |
| bl-s4-m11e-guinn-h1 | 24.2 | ACF #27 | Regicide as White | GuiNN | 80 | 8 | D | threefold repetition | winner moved into a non-won position at ply 117 (15-10, 1 such move); final db value D | `B:WK14,K19,21,27:B12,13,20,K29` |
| bl-s4-m11e-guinn-h1 | 25.1 | ACF #28 | Regicide as Black | GuiNN | 75 | 8 | D | threefold repetition | winner moved into a non-won position at ply 142 (30-25, 1 such move); final db value D | `W:WK9,14,15,22:B13,16,K19,K30` |
| bl-s4-m11e-guinn-h1 | 47.2 | ACF #55 | Regicide as White | GuiNN | 60 | 7 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `B:WK3,K9,24,28:B19,K21,25` |
| bl-s4-m11e-guinn-h1 | 72.2 | ACF #81 | Regicide as White | GuiNN | 66 | 8 | D | threefold repetition | winner moved into a non-won position at ply 81 (22-18, 1 such move); final db value D | `B:WK11,20,22,30:B12,13,21,28` |
| bl-s4-m11e-guinn-h1 | 8.2 | ACF #9 Wilderness I | Regicide as White | GuiNN | 50 | 7 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `B:WK11,16,27,31:B13,20,21` |
| bl-s4-m11e-guinn-h2 | 21.1 | ACF #111 | Regicide as Black | GuiNN | 74 | 8 | D | threefold repetition | winner moved into a non-won position at ply 84 (24-19, 1 such move); final db value D | `B:WK9,12,14,K19:B3,7,K10,K32` |
| bl-s4-m11e-guinn-h2 | 22.1 | ACF #112 | Regicide as Black | GuiNN | 56 | 8 | D | threefold repetition | still won at the end (final db value B, 7 pieces): ran out the rules | `B:WK8,11,18,19:B5,9,K10,12` |
| bl-s4-m11e-guinn-h2 | 4.2 | ACF #92 | Regicide as White | GuiNN | 69 | 8 | D | threefold repetition | winner moved into a non-won position at ply 103 (10-6, 1 such move); final db value D | `W:W11,K15,18,28:B19,24,K29,K30` |
| bl-s4-m11e-guinn-h2 | 43.2 | ACF #134 | Regicide as White | GuiNN | 64 | 7 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `B:W8,9,28,32:B10,20,K26` |
| bl-s4-m11e-guinn-h2 | 60.1 | ACF #153 Black Widow | Regicide as Black | GuiNN | 68 | 8 | D | threefold repetition | winner moved into a non-won position at ply 90 (11-7, 1 such move); final db value D | `B:W6,K21,28,32:BK15,19,20,K30` |
| bl-s4-m11e-guinn-h2 | 7.2 | ACF #96 Skull-Cracker | Regicide as White | GuiNN | 76 | 7 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `B:WK8,11,K19,21:B12,K26,K27` |
| bl-s4-m11e-guinn-h2 | 73.1 | ACF #167 | Regicide as Black | GuiNN | 68 | 8 | D | threefold repetition | still won at the end (final db value B, 7 pieces): ran out the rules | `B:W12,17,K18,23:B3,16,K19,28` |
| bl-s4-m11e-kingsrow-h1 | 24.1 | ACF #27 | Regicide as Black | KingsRow | 64 | 8 | D | threefold repetition | still won at the end (final db value B, 7 pieces): ran out the rules | `B:WK3,11,12,K17:B4,13,19,25` |
| bl-s4-m11e-kingsrow-h1 | 35.2 | ACF #39 | Regicide as White | KingsRow | 99 | 8 | D | threefold repetition | still won at the end (final db value W, 8 pieces): ran out the rules | `W:WK2,K7,11,15,28:BK21,27,K31` |
| bl-s4-m11e-kingsrow-h1 | 41.2 | ACF #47 | Regicide as White | KingsRow | 60 | 8 | D | threefold repetition | winner moved into a non-won position at ply 71 (32-27, 1 such move); final db value D | `B:W5,K14,31,32:B20,22,23,28` |
| bl-s4-m11e-kingsrow-h1 | 51.2 | ACF #59 | Regicide as White | KingsRow | 108 | 8 | D | threefold repetition | still won at the end (final db value W, 8 pieces): ran out the rules | `B:WK13,K14,K26,30:B5,K16,21,K27` |
| bl-s4-m11e-kingsrow-h1 | 52.2 | ACF #60 Black Hole | Regicide as White | KingsRow | 63 | 8 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `W:WK15,20,27,32:B9,K17,19,25` |
| bl-s4-m11e-kingsrow-h1 | 57.1 | ACF #65 Fraser's Inferno | Regicide as Black | KingsRow | 102 | 8 | D | threefold repetition | winner moved into a non-won position at ply 148 (32-27, 1 such move); final db value D | `B:WK15,K16,21,32:BK13,K22,23,K24` |
| bl-s4-m11e-kingsrow-h1 | 7.1 | ACF #8 Edinburgh Single | Regicide as Black | KingsRow | 68 | 8 | D | 40-move rule (no capture or man move) | winner moved into a non-won position at ply 146 (30-25, 1 such move); final db value D | `B:WK1,9,11,21:B3,5,14,K18` |
| bl-s4-m11e-kingsrow-h1 | 77.2 | ACF #86 Octopus | Regicide as White | KingsRow | 81 | 8 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `W:WK4,13,K17,30:BK18,21,24,K25` |
| bl-s4-m11e-kingsrow-h2 | 21.2 | ACF #111 | Regicide as White | KingsRow | 82 | 8 | D | 40-move rule (no capture or man move) | winner moved into a non-won position at ply 119 (7-2, 1 such move); final db value D | `B:WK6,19,K23,30:B5,12,21,K28` |
| bl-s4-m11e-kingsrow-h2 | 42.2 | ACF #133 Octopus | Regicide as White | KingsRow | 133 | 8 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `W:W8,K15,K18,K21:BK22,K30,K31,K32` |
| bl-s4-m11e-kingsrow-h2 | 58.1 | ACF #151 | Regicide as Black | KingsRow | 70 | 8 | D | threefold repetition | still won at the end (final db value B, 8 pieces): ran out the rules | `B:W10,K11,21,29:B13,22,K26,K30` |
| bl-s4-m11e-kingsrow-h2 | 6.2 | ACF #95 | Regicide as White | KingsRow | 75 | 8 | D | threefold repetition | still won at the end (final db value W, 7 pieces): ran out the rules | `W:W5,7,K12,13:B1,K8,20,21` |
| bl-s4-m11e-kingsrow-h2 | 64.2 | ACF #158 | Regicide as White | KingsRow | 100 | 8 | D | threefold repetition | still won at the end (final db value W, 8 pieces): ran out the rules | `B:WK9,15,20,28,K29:B18,27,K31` |

## 2. An opponent had a database win against Regicide and did not win (0)

| Run | Round | Ballot | Winner side | Against | First won ply | Pieces | Result | Termination | How it was lost | FEN of the first won position |
|---|---|---|---|---|---|---|---|---|---|---|

## 3. Opponent vs opponent: database win not converted (0)

| Run | Round | Ballot | Winner side | Against | First won ply | Pieces | Result | Termination | How it was lost | FEN of the first won position |
|---|---|---|---|---|---|---|---|---|---|---|

