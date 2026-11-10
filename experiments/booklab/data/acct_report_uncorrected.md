# Runner wall time minus host-reported getmove time, per move (old accounting)

Shim intervals in WSL seconds, uncorrected.

Shim interval: from the runner's `go` line reaching the shim to the host's `bestmove` line leaving it (the runner's own clock adds one pipe hop on each side). Position interval: from the `position ...` line (with the game history) instead. Runner charge: the time in the PDN comment (2 decimals). Only searches of positions played in the games count (the runner's warm-up search at every engine start is left out).

| Engine (match) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max | host S mean |
|---|---|---|---|---|
| Regicide (vs Cake) | 853 | -18.7 ms / 0.5 ms | -18.4 ms / 1.6 ms | 219 ms |
| Cake (vs Cake) | 852 | -22.7 ms / 0.4 ms | -22.4 ms / 1.0 ms | 269 ms |
| Regicide (vs KingsRow) | 1259 | -10.1 ms / 2.9 ms | -9.7 ms / 5.7 ms | 146 ms |
| KingsRow (vs KingsRow) | 1259 | -11.6 ms / 3.0 ms | -11.3 ms / 4.2 ms | 167 ms |
| Regicide (vs GuiNN) | 1021 | -16.5 ms / 0.5 ms | -16.2 ms / 1.8 ms | 232 ms |
| GuiNN (vs GuiNN) | 1017 | -21.7 ms / 0.6 ms | -21.4 ms / 1.9 ms | 304 ms |

| Engine (all matches) | moves | shim − S: mean / max | position-to-bestmove − S: mean / max |
|---|---|---|---|
| Regicide | 3133 | -14.5 ms / 2.9 ms | -14.2 ms / 5.7 ms |
| Cake | 852 | -22.7 ms / 0.4 ms | -22.4 ms / 1.0 ms |
| KingsRow | 1259 | -11.6 ms / 3.0 ms | -11.3 ms / 4.2 ms |
| GuiNN | 1017 | -21.7 ms / 0.6 ms | -21.4 ms / 1.9 ms |

Runner charge per move from the PDN comments (all moves incl. book moves): cake mean 246 ms, max 2.53 s (852 moves); guinn mean 282 ms, max 2.45 s (1017 moves); kingsrow mean 154 ms, max 4.24 s (1259 moves); regicide mean 179 ms, max 2.35 s (3133 moves)
