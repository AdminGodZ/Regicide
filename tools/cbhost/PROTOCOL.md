# cbhost line protocol

`cbhost` loads one CheckerBoard-API engine (a `.dylib`/`.so` exporting `getmove` and,
optionally, `enginecommand`) and drives it over stdin/stdout. CheckerBoard engines keep global
state and are not reentrant, so every engine instance runs in its own `cbhost` process. The
match runner only speaks this protocol, so any program that implements it can be used as an
engine. The Regicide CLI (`regicide`) does not implement it; Regicide plays matches as the
CheckerBoard library `regicide_cb` loaded by `cbhost`.

```
cbhost [--cwd DIR] [--no-gamehist] ENGINE.dylib
```

The working directory defaults to the engine's directory, because engines open their data
files (books, nets, endgame databases) relative to it. Anything the engine prints to stdout is
redirected to stderr, so stdout carries only protocol lines.

## Conventions

- One command per line, words separated by single spaces, `\n` line endings.
- Squares use PDN numbering 1..32. Black starts on 1..12 and moves first.
- Positions are PDN FEN: `B:W21-32:B1-12`, side to move first, `K` prefix for kings, ranges
  allowed. FENs contain no spaces.
- Moves are PDN: `11-15` for a quiet move, `15x22` or `1x10x19` for captures. cbhost always
  sends the full landing path of a capture, so a move is never ambiguous; the runner identifies
  moves by from/to/captured set.
- Text from the engine (`info`, `reply`, names) has newlines escaped as `\n`, backslashes as `\\`.

## Startup

After loading the engine, cbhost prints

```
id name <engine name>
id protocol cbhost 1
ready
```

or a single `error <message>` line and exits non-zero if the engine cannot be loaded.

## Commands

| Command | Reply | Meaning |
|---|---|---|
| `ping` | `pong` | Synchronisation. |
| `name` | `id name <name>` | Engine name (`enginecommand("name")`). |
| `cmd <text>` | `reply <0\|1> <text>` | Pass `<text>` to `enginecommand` verbatim, e.g. `cmd set hashsize 64`, `cmd get book`. `1` if the engine handled it. |
| `newgame` | `ok` | Start a new game: the next `go` sends `CB_RESET_MOVES`, and the move history is cleared. |
| `position startpos [moves <m1> <m2> ...]` | `ok` / `error <msg>` | Set the position. |
| `position fen <FEN> [moves <m1> ...]` | `ok` / `error <msg>` | Same, from a FEN. Moves are validated. |
| `go maxtime <s> [reset] [exact] [inc <s> remaining <s>]` | `info ...` then `bestmove ...` | Search the current position (see below). |
| `stop` | none | Ask the search of the last `go` to return now (sets `*playnow`). If that `go` has not reached the engine yet (it is queued, or the game history is being sent), the engine is entered with the flag set. A `go` read earlier is not affected, and each queued `go` keeps its own `stop`. Ignored when no `go` is outstanding. |
| `quit` | none | Exit (also on EOF). Every `go` that is still queued or running is stopped first: it enters the engine with `*playnow` set and its `bestmove` is still printed. |

Unknown commands get `error unknown command '<cmd>'`.

### go

`go` calls

```
getmove(board, color, maxtime, str, &playnow, info, moreinfo, &move)
```

with the arguments built the way CheckerBoard builds them:

- `maxtime <s>`: the `maxtime` argument, in seconds. The caller computes it (CheckerBoard's
  engine-match rules for fixed time per move bank unused time; see `tools/match`).
- `reset`: set `CB_RESET_MOVES` (also set automatically for the first `go` after `newgame`).
  CheckerBoard sets it on each side's first search of a game.
- `exact`: set `CB_EXACT_TIME`.
- `inc <s> remaining <s>`: incremental time control. The two times are packed into `info` bits
  2..3 and `moreinfo` exactly like CheckerBoard's `format_time_args()`.
- `info <n> moreinfo <n>`: raw override of both arguments.

Before each search cbhost sends the game history the way CheckerBoard does, so engines can see
repetitions: `enginecommand("set gamehist <FEN> <m1> <m2> ...")`, where `<FEN>` is the position
after the last conversion move (capture or man move) and the moves are the reversible king moves
since, as `from-to`. `--no-gamehist` disables this. The history is whatever `position ... moves`
supplied plus the moves played by `go`.

Replies:

```
info <engine status string>
bestmove <move> result <unknown|win|loss|draw> time <seconds>
```

`result` is the engine's own claim (`getmove`'s return value); the referee decides whether to
honour it. `time` is the wall time cbhost measured around `getmove`. The played move is
identified the way CheckerBoard does it: the board the engine wrote back is compared with every
legal successor; engines that leave the board untouched are matched through the `CBmove`
struct instead. If neither matches a legal move:

```
error illegal-move <details>
```

If the side to move has no legal move, `go` answers `bestmove none result loss time 0` without
calling the engine.

## Example

```
> position startpos moves 11-15 22-18
ok
> go maxtime 0.2 reset
info best:15x22 time 0.00s ...
bestmove 15x22 result unknown time 0.0001
> position fen B:W15,24:B10
ok
> go maxtime 0.1
info forced capture
bestmove 10x19x28 result unknown time 0.0000
```

## Implementing it natively

A native engine does not need cbhost: it just has to read these commands on stdin and print
these replies on stdout, flush after each line, and handle `stop` while searching (read stdin on
a separate thread). `go` may ignore `info`-style flags it does not need, but must honour
`maxtime`, `inc`/`remaining`, and `stop`.
