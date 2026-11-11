# A CheckerBoard engine match that equals the runner's reference profile

The runner's reference profile is one pair of engine processes playing the whole ordered deck at 0.5 s per move with
CheckerBoard's claim handling (Book Lab's mo2 runs). This page says how to set up the same match in the CheckerBoard
program, and what still differs. Source: reviewer A, rounds 5 and 8 (`reviews/a/round5`, `reviews/a/round8` on
`t3/review-reports`), read against CheckerBoard 1.78 (public revision 32f194f).

## Settings in a freshly started CheckerBoard

1. **The same DLL.** Select the same 64-bit Regicide DLL (same embedded book, net and avoid list) as primary engine.
   A freshly loaded DLL already has book 3 (best moves), hash 128 MB and one search thread; CheckerBoard does not
   change them when it loads the engine or starts a match.
2. **The endgame database beside the DLL.** Regicide loads its tables from the folder `egdb` next to its DLL, not
   from CheckerBoard's database setting and not from a KingsRow-format folder. Put the matching Regicide tables there
   (or send `set dbpath PATH` before any other folder has loaded). Check with the engine command dialog:
   `get dbpath` must name that folder and `get dbpieces` must answer 8 for the full set; an empty path or 0 means no
   tables. A loaded folder cannot be exchanged in a running DLL: unload it or restart CheckerBoard.
3. **Hash 128, book 3, threads 1.** These are the defaults of a fresh load; no setting is needed. If the DLL was
   used and changed before in the same session, set hash 128 MB and book mode 3 again in the engine options, and
   send `set threads 1` in the engine command dialog (the GUI's search-threads control is disabled for Regicide).
   Regicide stores no settings between sessions.
4. **Time and adjudication.** Time level 0.5 s per move (the fresh default is 2 s), not incremental, handicap off.
   The exact-time box does not matter in engine matches. Early adjudication on (the default).
5. **Openings and match.** GUI user book off; openings cross-board and mail-play on, barred off (the 156 ballots of
   our deck, in the same order); repeat 1; a fresh match, not a resumed one. The primary engine plays Black first,
   then each ballot again with colours reversed. Give the opponent the same book, hash, database and thread settings
   as in the runner's options.

## The corresponding runner options

| CheckerBoard | Runner |
|---|---|
| one DLL pair for the whole match | `-c 1 --pairs-together`, processes kept (no `--fresh-engines`) |
| 0.5 s level, non-incremental | `--movetime 0.5` (default `cb` time mode) |
| early adjudication on: draw, win and loss claims end the game; 200 searches per game | `--cb-claims --cb-move-limit 200` |
| 156 ballots, cross-board and mail-play | `--openings acf156` (or the same deck as a file) |
| Regicide defaults | `--option-a "set hashsize 128" --option-a "set threads 1" --option-a "set book 3"` |

## Differences that remain

- **Warm-up.** CheckerBoard gives each engine one 0.1 s search on the starting board without the reset flag at match
  start; the runner gives one 0.05 s search with the reset flag when it starts a process.
- **Repetition boundary.** CheckerBoard's repetition check leaves out the oldest position of the reversible
  stretch; the runner counts it. The runner can therefore call a third occurrence one ply earlier.
- **Time-outs.** CheckerBoard never stops an engine or scores a time loss in a fixed-time match. The runner sends
  `stop` after 4 × maxtime + 0.25 s and kills the engine 2 s (or maxtime, if longer) later; that game is lost.
- **Time arguments.** The time policy is the same (a bank per colour and game, the same maxtime formula), but the
  runner passes maxtime as text with four decimals, and both measure elapsed time with their own clocks.

These are accepted; no setting removes them. The runner and CheckerBoard also differ in process layout (separate
host processes against both DLLs in one program) and working directory, which matters only for engines that read
files relative to it.
