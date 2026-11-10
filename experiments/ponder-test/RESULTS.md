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

### Setup

- Opponent settings as in the baselines (`run_ba3.sh`): own books, `set dbpath C:\regicide-match\db\kingsrow_wld8`,
  `enable_wld 1`, `dbmbytes 512`, `max_dbpieces 8`, `hashsize 128`; KingsRow `searchthreads 1`. acf156 (312 games),
  0.5 s/move (CheckerBoard match rules: unused time is banked), `--pdn-comments`.
- Regicide `ponder-df7780a` (egdb 2-8 via junction). **Book off** (`set book 0`) in A/B/C; the avoid list stays at
  its default (on, built-in, 6 entries). Configurations:

  | cfg | threads | ponder | hash | -c | lock |
  |---|---|---|---|---|---|
  | A | 1 | 0 | 128 | 8 | timing lock only |
  | B | 1 | 1 | 128 | 8 | timing lock + matchlock |
  | C | 4 | 1 | 512 | 4 | timing lock + matchlock |

- Series (`scripts/chain.sh`): A1 B1 C1 A2 B2 C2 vs Cake, then C with the book on (DLL default, book mode 3) vs
  Cake, KingsRow and GuiNN. Each run took `~/regicide-jobs/locks/timing.lock` by itself, so Book Lab's and Truth
  data's runs ran in between (2026-11-09 23:30 to 2026-11-10 06:09). `scripts/analyze.py` produces the numbers below
  (`logs/analysis.txt`). Per-run `summary.txt`, `results.csv`, `losses.pdn` are in `results/`.
- **Searched moves** (for depth, speed): Regicide moves with 2 <= depth < 127 and >= 1,000 nodes (no book, forced
  or database-decided root moves). For a ponder hit, the depth and nodes are the ponder search's, including the time it
  spent on the opponent's clock; its kN/s is still nodes per second of search.
- **Hit rate:** all `ponder hit` / (`hit` + `miss`), and in brackets only where the opponent took >= 50 ms over its
  reply (instant book moves and forced captures are "misses" logged 0-1 ms after pondering started and cost nothing).

### Stability

**No time forfeits, crashes, hangs or engine kills in any of the 10 runs (2,848 games incl. the smoke run).** Every
game ended by repetition, the 40-move no-progress rule or no legal moves (a normal win/loss), and every run exited with
rc 0. Regicide's longest single move: 4.8 s (A), 5.8 s (B), 5.5 s (C), under CheckerBoard's banking rules, against
Cake's own maximum of 3.9-5.6 s in the same runs. Every matchlock I created was removed at the end of its run
(`logs/status.txt`).

### Per run

| run | cfg | book | opp | W/D/L | Elo (pentanomial) [95% CI] | losses (ballot, Regicide's colour) | med. depth | med. kN/s | hit rate | ms/hit | wall | load at start / background |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| A1 | A | off | Cake | 0/307/5 | -5.6 [-10.4, -0.8] | #153 Black Widow (W), #5 Dreaded Edinburgh (B), #62 Gemini I (B), #7 (B), #89 (B) | 20 | 2,142 | - | - | 12 min | 8.3; **no matchlock**: Truth's GPU training (python, R) and rescore-tr1 (S, not stopped) running |
| B1 | B | off | Cake | 0/307/5 | -5.6 [-10.4, -0.8] | #129 (W), #153 (W), #154 (B), #16 Wilderness II (B), #62 (B) | 22 | 3,150 | 42.3% (71.0%) | 373 | 11 min | 7.3; matchlock, rescore stopped |
| C1 | C | off | Cake | 0/308/4 | -4.5 [-8.8, -0.1] | #10 (B), #153 (W), #5 (B), #62 (B) | 25 | 11,183 | 40.4% (66.0%) | 371 | 22 min | 7.5; matchlock, rescore stopped, GPU training (python, R) |
| A2 | A | off | Cake | 0/307/5 | -5.6 [-10.4, -0.8] | #129 (W), #153 (W), #62 (B), #64 (B), #7 (B) | 21 | 3,024 | - | - | 11 min | 9.0; no matchlock, rescore-tr1 stopped (T) at the start |
| B2 | B | off | Cake | 0/305/7 | -7.8 [-13.5, -2.1] | #112 (W), #129 (W), #153 (W), #6 Inferno (B), #62 (B), #64 (B), #7 (B) | 23 | 3,219 | 43.2% (70.3%) | 378 | 11 min | 7.7; matchlock, rescore stopped |
| C2 | C | off | Cake | 0/307/5 | -5.6 [-10.4, -0.8] | #14 (B), #153 (W), #34 Twilight Zone (B), #53 (W), #6 (B) | 25 | 11,494 | 41.6% (67.7%) | 381 | 22 min | 5.4; matchlock, rescore stopped |
| C-on | C | on | Cake | 0/309/3 | -3.3 [-7.1, +0.4] | #153 (W), #62 (B), #87 Tyne (B) | 25 | 11,550 | 42.2% (68.1%) | 365 | 22 min | 7.0; matchlock |
| C-on | C | on | KingsRow | 0/311/1 | -1.1 [-3.3, +1.1] | #153 (W) | 25 | 11,518 | 39.4% (71.8%) | 278 | 27 min | 4.6; matchlock |
| C-on | C | on | GuiNN | 1/310/1 | 0.0 [-3.1, +3.1] | #157 (B) | 25 | 11,614 | 43.0% (66.6%) | 371 | 24 min | 7.4; matchlock |

(datagen-eb1037c was stopped (T, `gen4bhold`) in every run. Load = 1-min load average just before the match.)

### A / B / C pooled (2 x 312 games each, book off, vs Cake)

| cfg | W/D/L | pentanomial [LL LD DD/WL WD WW] | Elo [95% CI] | vs A [95% CI] | losses B / W | hit rate | med. depth (hits / other) | med. kN/s | med. nodes | wall per 312 games |
|---|---|---|---|---|---|---|---|---|---|---|
| A: 1 thread, no ponder | 0/614/10 | 0 10 302 0 0 | **-5.6 [-9.0, -2.2]** | - | 7 / 3 | - | 21 | 2,512 | 1.26 M | 11-12 min at -c 8 |
| B: 1 thread, ponder | 0/612/12 | 0 12 300 0 0 | **-6.7 [-10.4, -3.0]** | **-1.1 [-6.2, +3.9]** | 7 / 5 | 42.7% (70.7%) | 22 (23 / 21) | 3,183 | 2.10 M | 11 min at -c 8, machine-wide quiet |
| C: 4 threads, ponder | 0/615/9 | 0 9 303 0 0 | **-5.0 [-8.2, -1.8]** | **+0.6 [-4.1, +5.2]** | 6 / 3 | 41.0% (66.9%) | 25 (25 / 22) | 11,340 | 7.25 M | 22 min at -c 4, machine-wide quiet |

Losses by ballot, pooled:

- **A:** #153 Black Widow (W) x2, #62 Gemini I (B) x2, #7 (B) x2, #129 (W), #5 Dreaded Edinburgh (B), #64 (B), #89 (B).
- **B:** #153 (W) x2, #129 (W) x2, #62 (B) x2, #112 (W), #154 (B), #16 Wilderness II (B), #6 Inferno (B), #64 (B),
  #7 (B).
- **C:** #153 (W) x2, #10 (B), #14 (B), #34 Twilight Zone (B), #5 (B), #53 (W), #6 (B), #62 (B).

**#153 Black Widow with Regicide as white was lost in all 8 Cake/KingsRow runs (A1-C2, C-on vs Cake, C-on vs
KingsRow). #62 Gemini I as black was lost in 6 of the 7 Cake runs.** Four times the threads plus pondering do not fix
these, so they are opening problems for the book or avoid list, not a search-depth problem. The remaining losses are
scattered single ballots that change from run to run.

### Reading

- **Pondering and threads do not reduce the losses to Cake.** A, B and C are all about -5 to -7 Elo, and the
  differences to A are +-1 Elo with CIs of about +-5 (B -1.1 [-6.2, +3.9], C +0.6 [-4.1, +5.2]). Losses per 624 games:
  A 10, B 12, C 9. This confirms the lead's expectation (4x thinking time was worth about 3 Elo against these
  opponents).
- **The search does get stronger, as intended.** About 70% of the replies Cake actually thinks about are predicted,
  ~40% overall because Cake plays many instant book or forced moves. With ponder the median searched depth goes from 21 to
  22 (hits 23); with 4 threads + ponder it goes to 25 (hits 25, others 22) at 4.5x the nodes per second (11.3 vs 2.5
  MN/s). A hit typically answers at once with a search that had already run ~370 ms on the opponent's time. At this draw
  rate (98-99%) the extra depth does not turn into points.
- **Book on, configuration C** vs Cake -3.3 [-7.1, +0.4] (3 losses), KingsRow -1.1 [-3.3, +1.1] (1 loss, #153), GuiNN
  0.0 [-3.1, +3.1] (1 win, 1 loss). For comparison, the 1-thread book_deep2b runs on 6bb8c6c with the same book
  (`ba-deep2b-full-*`, older engine commit, no ponder) scored -5.6 [-10.4, -0.8] vs Cake (5 losses) and 312 draws vs
  KingsRow. Same picture: within noise.
- **Throughput cost.** B needs a quiet machine (matchlock: background generators stopped) for the same wall time as
  A. C needs the matchlock and twice the wall time (-c 4 instead of 8) while keeping ~20 of 24 cores busy, about 2.5x
  the CPU of A per game. A without the matchlock also measures noise from the background: A1's median speed (2.1
  MN/s, Truth's rescore and training running) was 30% below A2's (3.0 MN/s, rescore stopped). The Elo result was the
  same.
- **Recommendation:** keep testing in the cheap 1-thread, no-ponder setup (A). Turn on threads + ponder only for final
  or showcase runs (stable, no forfeits, no hangs), and not as a way to fix the Cake losses. Those are concentrated
  in #153 Black Widow (W) and #62 Gemini I (B), which are book work.

### Not done / caveats

- No run used more than 312 games per configuration pair-set; the A/B/C differences are resolved to about +-5 Elo,
  not better.
- The book-on comparison uses older 1-thread runs (6bb8c6c, before tune2/fcf6acb), not a fresh 1-thread book-on run
  of this DLL; no extra runs were made, so as not to hold the shared lock.
- A1/A2 ran without the matchlock, as specified, so their background differed from B/C (see the load column).
