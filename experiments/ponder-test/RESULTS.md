# ponder-test: pondering and threads in the CheckerBoard DLL on Windows

- **Engine:** `t3/ponder` df7780a (= master fcf6acb + pondering), built on Win11-AdminGod (Core Ultra 9 285K, 24
  cores) in WSL2 with llvm-mingw clang 23.1.5 (ucrt), the same toolchain file and flags as the master DLLs
  (`tools-src/tools/windows/mingw-clang.cmake`, Release, `-static`). The embedded book is master's
  `src/weights/book.bin` (= book_deep2b, sha256 e72e384f425f9d58). DLL directory
  `C:\regicide-match\engines\regicide\ponder-df7780a`; its `egdb` is a directory junction to
  `..\6bb8c6c-deep2b-egdb8\egdb` (2-8 pieces, no new 11 GB copy).
- **Match runner:** `match` built from the same commit (`~/regicide-match/bin/match-ponder-df7780a`; prints the
  ponder hit rate), cbhost `C:\regicide-match\bin\b71dfa9\cbhost.exe` as before.
- Scripts: `scripts/` (build, install, cbhost checks, unload check, match runner, series chain, analysis).

## Step 1: build and verify on Windows

### Build: no fixes were needed

The Windows-specific code (`PonderThread` with `GetModuleHandleExA(FROM_ADDRESS)` + `CreateThread` +
`FreeLibraryAndExitThread`, `PonderReaper` detaching on Windows) compiled unchanged. The only warning in the whole
Windows build is the existing one in `src/egdb/store.cpp:215` (`total` set but not used). Nothing was committed to
`t3/ponder`. Log: `logs/build-windows.log`.

- Linux build of the same tree: ctest **21/21**, `regicide bench` **7,712,020** (unchanged).

### Native Windows tests (`ponder_test.exe`, cross-compiled, run from PowerShell)

All 8 cases pass (`logs/ponder_test-windows.txt`):

| case | result |
|---|---|
| hit | pass (hit after 1000 ms: answered in 0.12 ms at depth 18; hit at once: 128 ms, normal timed search) |
| predict | pass (book move, reply predicted by the short search, hit) |
| miss | pass (39 ms) |
| stop | pass (worst stop latency 0.23 ms) |
| newgame | pass (51 ms, "ponder miss") |
| teardown | pass (FreeLibrary while pondering 0.006-0.023 ms; "the library stayed loaded", see below) |
| exit | pass |
| games | pass (4 games, 31 hits, 38 misses, every move legal) |

### Unloading (`scripts/unload_check.cpp`, `logs/unload_check.txt`)

Load, `set ponder 1`, `set threads 4`, one 0.5 s `getmove`, wait until it ponders, `FreeLibrary`:

- `FreeLibrary` returns at once (0.00 ms); the module stays mapped while the ponder thread holds its reference.
- The module is **really unmapped 4.7 s later**, when the ponder search hits its own cap (10 x the move's 0.5 s
  budget) and the thread leaves through `FreeLibraryAndExitThread`. The DLL's destructors then run on that thread
  with nothing to wait for (search helper threads are per search and already joined), so no loader-lock deadlock.
- A new `LoadLibrary` after that starts clean (`get ponder` = 0). A reload *while still pinned* gets the same
  instance (`get ponder` = 1) and works: its first `getmove` stops the old ponder search (a miss) and plays in 173 ms;
  the next `FreeLibrary` unmaps the module at once.

**Side effect (by design, worth knowing):** on Windows, unloading the engine during pondering does not stop the
ponder search; it keeps its threads busy until its cap (10 x the last move's time, at least 1 s) and only then
unloads. In CheckerBoard with long time controls that is up to 10 x the move time of CPU after switching engines.
Process exit is not affected (Windows ends the thread; `exit` test and cbhost `quit`/EOF below).

### cbhost checks (`scripts/cbcheck.py`, `logs/cbcheck.txt`): 12/12 pass

- ponder is off by default; with `ponder 0` the moves come in time, without a ponder note, and no ponder search
  runs after the move (`get ponderpos` empty); `quit` 0.11 s.
- `ponder 1`, `threads 4`, hash 512: the ponder search starts after the move; the predicted reply is a **hit**
  (answered at once, depth 21, 589 ms pondered); another reply is a **miss**, answered in time.
- **New game while pondering**: the next search answers in 0.23 s with "ponder miss"; 10 new games right after a
  move: worst 0.12 s at 0.1 s/move. An `enginecommand` while pondering stops it (0.14 s).
- `quit` and **EOF** (the match runner closing the pipe) while pondering: the process exits in 0.16 s, rc 0.

### Smoke match: configuration C vs Cake (40 games)

`run_p.sh smoke-c cake C 0 20`: threads 4, ponder 1, hash 512, book off, acf156 first 20 pairs, 0.5 s/move, -c 4,
timing lock + matchlock held.

| games | W/D/L | Elo (pentanomial) | forfeits / crashes / hangs | hit rate |
|---|---|---|---|---|
| 40 | 0 / 39 / 1 | -8.7 [-25.3, +7.9] | none (all 40 games ended normally: 39 repetition, 1 no legal moves) | 43.7% (598 / 1369); **68.3%** where Cake took >= 50 ms |

Loss: ACF #6 Inferno (Regicide black). Regicide 0.183 s/move on average, max 4.19 s (CheckerBoard banking rules).

**Hit rate:** most misses happen when Cake answers instantly (book moves, forced captures). Those "misses" are
logged 0-1 ms after pondering started and cost nothing. Counting only replies where Cake took at least 50 ms, the
hit rate is 68%, like the macOS figure (60-69%).

Searched moves are moves that are not book (depth 0), not forced (depth 1), not decided by the database at the root
(depth 127), and have at least 1,000 nodes. Their medians: depth 25, 9.3 MN/s (4 threads); the hits reach depth 25 vs
22 for the other moves. For comparison, the old 1-thread book-on run vs Cake (`ba-deep2b-full-cake`) has median depth
21 at 2.6 MN/s.

## Step 2: measurement

Pending (series queued: `chain.sh`, A B C A B C vs Cake with the book off, then C with the book on vs Cake, KingsRow
and GuiNN; each run takes the timing lock by itself).
