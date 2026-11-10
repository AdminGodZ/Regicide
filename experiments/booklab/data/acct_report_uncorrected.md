# Runner wall time minus host-reported getmove time, per move (old accounting)

Shim interval: from the runner's `go` line reaching the shim to the host's `bestmove` line leaving it (the runner's own clock adds one pipe hop on each side). Position interval: from the `position ...` line (with the game history) instead. Runner charge: the time in the PDN comment (2 decimals).

| Engine (match) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max | host S mean |
|---|---|---|---|---|
| Regicide (vs Cake) | 861 | -18.6 ms / 0.5 ms | -18.3 ms / 1.6 ms | 217 ms |
| Cake (vs Cake) | 860 | -23.8 ms / 0.4 ms | -23.5 ms / 1.0 ms | 279 ms |
| Regicide (vs KingsRow) | 1267 | -10.1 ms / 2.9 ms | -9.7 ms / 5.7 ms | 146 ms |
| KingsRow (vs KingsRow) | 1267 | -12.1 ms / 3.0 ms | -11.8 ms / 4.2 ms | 174 ms |
| Regicide (vs GuiNN) | 1029 | -16.4 ms / 0.5 ms | -16.1 ms / 1.8 ms | 230 ms |
| GuiNN (vs GuiNN) | 1025 | -22.3 ms / 0.6 ms | -22.0 ms / 1.9 ms | 312 ms |

| Engine (all matches) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max |
|---|---|---|---|
| Regicide | 3157 | -14.5 ms / 2.9 ms | -14.1 ms / 5.7 ms |
| Cake | 860 | -23.8 ms / 0.4 ms | -23.5 ms / 1.0 ms |
| KingsRow | 1267 | -12.1 ms / 3.0 ms | -11.8 ms / 4.2 ms |
| GuiNN | 1025 | -22.3 ms / 0.6 ms | -22.0 ms / 1.9 ms |

Runner charge per move from the PDN comments (all moves incl. book moves): cake mean 246 ms, max 2.53 s (852 moves); guinn mean 282 ms, max 2.45 s (1017 moves); kingsrow mean 154 ms, max 4.24 s (1259 moves); regicide mean 179 ms, max 2.35 s (3133 moves)
