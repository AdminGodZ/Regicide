# T1 report: tm_soft 100 minus 60, kingsrow, production deck, repeats a

- tm_soft 60: time forfeits 0; losses 0
- tm_soft 100: time forfeits 0; losses 1
  - ACF #34 Twilight Zone (Regicide B): score below −100 from ply 17; black has no legal moves; bl-t1a-100-kingsrow-h1 round 31.1

- tm_soft 60: Regicide W / D / L = 1 / 311 / 0
- tm_soft 100: Regicide W / D / L = 0 / 311 / 1

**Paired score difference (100 − 60): -1.0 match-score points over 312 games per arm** (-0.32 percentage points); 95% interval resampling the 156 openings [-2.5, +0.0].

Regicide's time on searched moves (host time > 0.02 s) by pieces on the board: mean host time, median share of the offered maxtime [moves]:

| Pieces | tm_soft 60 | tm_soft 100 |
|---|---|---|
| 21-24 | 0.38 s, 43% [602] | 0.55 s, 71% [613] |
| 17-20 | 0.54 s, 43% [1095] | 0.69 s, 71% [1110] |
| 13-16 | 0.59 s, 40% [1214] | 0.67 s, 65% [1222] |
| 9-12 | 0.61 s, 38% [1468] | 0.67 s, 63% [1342] |
| <=8 | 0.50 s, 37% [25] | 0.53 s, 59% [10] |

Runner's bank before Regicide's first move with 8 or fewer pieces (this move's credit included), median / mean [games]:

- tm_soft 60: 3.78 s / 3.77 s [303]
- tm_soft 100: 2.40 s / 2.44 s [306]
