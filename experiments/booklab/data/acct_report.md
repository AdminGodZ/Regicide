# Runner wall time minus host-reported getmove time, per move (old accounting)

Shim intervals multiplied by 1.0788 (Windows seconds per WSL second) before the comparison.

Shim interval: from the runner's `go` line reaching the shim to the host's `bestmove` line leaving it (the runner's own clock adds one pipe hop on each side). Position interval: from the `position ...` line (with the game history) instead. Runner charge: the time in the PDN comment (2 decimals). Only searches of positions played in the games count (the runner's warm-up search at every engine start is left out).

| Engine (match) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max | host S mean |
|---|---|---|---|---|
| Regicide (vs Cake) | 853 | -3.0 ms / 92.6 ms | -2.6 ms / 93.1 ms | 219 ms |
| Cake (vs Cake) | 852 | -3.3 ms / 121.1 ms | -3.0 ms / 121.5 ms | 269 ms |
| Regicide (vs KingsRow) | 1259 | 0.6 ms / 11.1 ms | 1.0 ms / 11.5 ms | 146 ms |
| KingsRow (vs KingsRow) | 1259 | 0.6 ms / 15.7 ms | 1.0 ms / 16.0 ms | 167 ms |
| Regicide (vs GuiNN) | 1021 | 0.4 ms / 7.9 ms | 0.8 ms / 8.2 ms | 232 ms |
| GuiNN (vs GuiNN) | 1017 | 0.5 ms / 11.8 ms | 0.9 ms / 12.2 ms | 304 ms |

| Engine (all matches) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max |
|---|---|---|---|
| Regicide | 3133 | -0.4 ms / 92.6 ms | -0.1 ms / 93.1 ms |
| Cake | 852 | -3.3 ms / 121.1 ms | -3.0 ms / 121.5 ms |
| KingsRow | 1259 | 0.6 ms / 15.7 ms | 1.0 ms / 16.0 ms |
| GuiNN | 1017 | 0.5 ms / 11.8 ms | 0.9 ms / 12.2 ms |

Runner charge per move from the PDN comments (all moves incl. book moves): cake mean 246 ms, max 2.53 s (852 moves); guinn mean 282 ms, max 2.45 s (1017 moves); kingsrow mean 154 ms, max 4.24 s (1259 moves); regicide mean 179 ms, max 2.35 s (3133 moves)
