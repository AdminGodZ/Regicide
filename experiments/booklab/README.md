# booklab: opening knowledge from real games (Windows box, 2026-11-09/10)

> **Archived job scripts.** `scripts/` and the status logs (`timeline.txt`) are the historical jobs of this run on one
> machine (paths under `/mnt/c/regicide-match` and `/home/admingod`, CheckerBoard DLLs, a shared timing lock). They
> document how the results were produced; they are not general tooling. The reusable tools are in `tools/booklab/`.

**Status: work in progress** (this push: after WP3/WP4; census and validation still running). Final tables follow.

Master fcf6acb = engine 6bb8c6c + book_deep2b (bench 7,712,020; 8,089,571 with `set book off`). All match settings as
the baselines: 0.5 s/move (CheckerBoard rules), `-c 8`, hash 128, opponents with their own books and the KingsRow
8-piece WLD database, Regicide with its 2–8-piece database, PDN comments on. Each 312-game acf156 run is played as two
halves of 78 ballots (`scripts/acf156_h1.txt`, `acf156_h2.txt`), one machine-wide timing-lock hold per half.

## Tools (tools/booklab)

- `replay.cpp` → `booklab_replay`: replays PDN move lists with the engine's own rules (Position, parse_move); prints
  position keys, FENs and canonical moves.
- `theory.py build`: parses every match `games.pdn` on the machine (per-move comments: time, book marks, scores),
  replays them and builds an SQLite theory table keyed by position (FEN incl. side to move):
  `theory(key, family, move, n, n_instant, n_book, w, d, l)` with W/D/L from the mover's side; families cake,
  kingsrow, guinn (top engines) and regicide (all versions, kept separate); ballot moves excluded.
  `theory.py alts` shows the top engines' moves along a game; `theory.py repair` (WP5a) finds the first position of a
  lost game where Regicide's move is not one of the opponent's own non-losing moves there, and emits the opponent's
  defence from that position (every non-lost game in which it defended it, our side's moves up to 40 plies or 8
  pieces) as force decisions; `theory.py export` writes the opening part as CSV.
- `repair_all.py`: `repair` for every (ballot, colour, opponent) Regicide lost in given result dirs.
- `sweep.py`: preventive check of the positions the current Regicide reaches: its move has a losing record (all
  versions) and the top engines play another move there that never lost.
- `census.py`: WP1 tables. `losslines.py`: lost (or won) games grouped by ballot, colour and line.
- `bookapply.cpp` → `booklab_apply` (WP4): master's book + `decisions.txt` → book binary; every decision is verified
  after writing (`Book::probe` returns the forced move; `Book::avoids` for avoid lines).
- `tools/bookgen.cpp` (WP3): `--hash MB` (default 64), `--egdb DIR` (+ `--egdb-cache MB`, default 4096),
  `--lines FILE` (every position on each line is analysed, whatever its priority or ply, and kept on `--resume`).
  ctest 13/13; bench unchanged.

## decisions.txt (WP4)

One line per decision: `FEN | move | force|avoid | evidence`. Built with `scripts/assemble.sh` from the repair
outputs in `repairs/` (first decision per position wins) and turned into the book with `scripts/make_book.sh`
(base: master's `src/weights/book.bin` = book_deep2b). `force` replaces every book move of the position by the given
move (any ply; the engine probes by position key). DLLs are built by `scripts/bl_build_dll.sh` with the egdb folder
as a directory junction to an existing copy (no new 11 GB copies).

## Progress so far

- **Black Widow (W) vs Cake:** Regicide's book plays 27x18 at the first move after the ballot (lost 183 of 192 games
  vs Cake in the theory table). Cake, KingsRow and GuiNN all play 26x19 (Cake 196 draws in 196). Repair: force 26x19
  and Cake's defence tree (146 positions).
- **ACF #7 (B) vs Cake:** Regicide follows Cake's own defence until 19 plies after the ballot, then plays 2-7 (Cake:
  3-7, 67 draws in 67; Regicide 2-7: 60 losses in 61). Repair: 3-7 + Cake's defence (59 positions).
- **Gemini I (B):** deviation 5 plies after the ballot (16-20; Cake 7-10, 39 draws). **#79 / #90 (B)** transpose;
  deviation 19 plies after the ballot (5-9; Cake 1-6).
- Round 1 (`bl-r1`, these 5 repairs) vs Cake, 10 games per ballot and colour: **100 draws in 100** (master: Black
  Widow 59/60 lost, #7 39/60). vs KingsRow, 4 per ballot and colour: 40 draws.
- GuiNN census losses (#59 B, #63 B): deviations 1 and 9 plies after the ballot; there Cake, KingsRow and GuiNN all play
  the same other move with no losses, while Regicide's moves lost 16/47 and 13/18 in the theory table.
