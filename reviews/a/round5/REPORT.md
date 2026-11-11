# Review A — Round 5

Date: 2026-11-11. Local report: /home/admingod/regicide-review-a/reports/round5/REPORT.md.

**None of the current run types exactly reproduces the published CheckerBoard engine-match implementation.** Concurrency 1 with kept processes and consecutive colour-reversed pairs reproduces its opening order and long process history, and is the appropriate base for comparison. Differences remain in claims, game-length handling, a repetition boundary, warm-up, and host/environment details. Eight-worker runs do not reproduce one CheckerBoard match's history, even with --pairs-together.

Two concrete compatibility findings are numbered below. Other differences are listed with their scope and possible effect; they are not presented as proven causes of the experimental results.

The remaining experiment-2 and concurrency-1 PDNs have not been committed at the final Book Lab head checked. Available CSV results are recounted below. I have not invented process ages from run names.

## Scope, sources and execution

I fetched first and read docs/REVIEW_LEDGER.md from origin/master. Accepted or assigned findings are not reported again.

| Source | Revision examined |
|---|---|
| Regicide origin/master | 12f0e09d034d09f4dbc9850f29f9f54cb235f940 |
| Regicide origin/t3/booklab | b9a0f35fb196ffc42b5a8de75828ca9be208724b |
| Book Lab c1 launch recipe | introduced in 223e6af; relevant commands unchanged at b9a0f35 |
| Public eygilbert/CheckerBoard master | 32f194f840f208f0aff86a07026f788c01c819ce; source version 1.78 |
| Prior process-state evidence | Review A round 4: 28 tagged PDNs, 3,360 games |

I cloned CheckerBoard into this review directory. checkerboard.vcxproj compiles the root CheckerBoard.c and utility.c; those are traced here, rather than an unused copy. The answers describe that public revision. I did not identify, start or observe an installed GUI binary, and cannot certify an older installation.

Ran under nice -n 19, foreground, in my private directory:

- A standalone build of current master match and cbhost, at most two build jobs.
- A private scripted CheckerBoard API library through the real Linux cbhost and runner, one fixture game at a time. No actual opponent engine/database was loaded.
- An extracted-source harness compiling CheckerBoard's actual is_conversion_move, detect_nonconversion_draws, send_game_history and fixed-time allocator, with portable board/UI adapters.
- A second extracted-source harness for update_match_stats and its result-string conversion.
- Comparison of all 156 ballots between the reference table, our embedded deck and committed Book Lab half-decks.
- CSV recounts and comparison of existing PDN hashes with the round-4 manifest.

Read only: Windows match controller/loader/timer, help/API text, our Windows loader, production launch and affinity scripts. No GUI build/run, production script, lock, real engine match, other agent's process or directory was touched. Fixtures used no search workers. Everything generated stays in /home/admingod/regicide-review-a; only report text is committed.

## Finding 1 — Book Lab's runner calls do not apply CheckerBoard's claim policy

Regicide: tools/match/main.cpp:320, play_game; Config at :74.

Reference: CheckerBoard.c, [SearchThreadFunc result handling](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L3573).

CheckerBoard always ends a match game after a legal move returning CB_DRAW. With early adjudication enabled, its default, it also ends after CB_WIN/CB_LOSS, from the calling engine's perspective. With that option disabled it still accepts draws but continues past win/loss claims.

Our host preserves these return values. The runner ignores win/loss claims in every configuration and accepts draws only with --accept-draw-claims. Book Lab's bl_run_w.sh/mo3_run.sh do not pass that option.

Concrete executed fixture:

~~~text
Opening: B:WK32:BK1
--movetime 0.5 --pairs 1 --pairs-together -c 1 --max-plies 12
Both mock DLLs return a legal move and the selected getmove result.
Moves: 1-5 32-28 5-1 28-32, repeated
~~~

| Returned result | Runner without claim option | Reference CheckerBoard |
|---|---|---|
| CB_UNKNOWN | repetition draw after 8 plies | detector first fires at ply 9; finding 2 |
| CB_DRAW | ignores claim, draws after 8 plies | draw after the first move |
| CB_WIN | ignores claim, draws after 8 plies | Black win after first move if early adjudication enabled |
| CB_LOSS | ignores claim, draws after 8 plies | White win after first move if early adjudication enabled |

Each fixture opening was played twice with engine slots reversed. --accept-draw-claims makes the draw case end after one ply; it does not implement win/loss claims.

Runner results were executed. Reference claim results follow from the inspected branch, not a GUI run. The arbitrary claims test the host contract, not the position's truth.

There is no fixed Elo bias. Ignoring a correct concession can turn a win into a draw if conversion fails; ignoring an incorrect win claim can prevent a false result. Ignoring draws permits more play and changes state for the next game, even if the score remains drawn. These are different measurement policies.

## Finding 2 — repetition differs at the reference window boundary

Regicide: tools/match/main.cpp:306, play_game.

Reference: utility.c, [detect_nonconversion_draws](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/utility.c#L714).

Concrete legal input:

~~~text
Start: B:WK32:BK1
1-5 32-28 5-1 28-32 1-5 32-28 5-1 28-32
~~~

At ply 8, the initial board with Black to move has occurred three times. Our runner includes that start boundary and declares a draw.

CheckerBoard examines same-side distances strictly less than the reversible count. At count 8 it sees distance 4 but excludes distance 8, and continues. If Black then plays 1-5, the ply-9 board has earlier occurrences at distances 4 and 8, now both inside the window, and it draws.

Ran the real runner/cbhost with the legal-cycle DLL: both games draw at 8 engine plies. Ran the actual reference detector compiled with portable adapters: false at 8, true at 9. The extracted history sender includes the initial FEN and all eight moves: history supplied to an engine is not missing that board.

This is compatibility with the reference's boundary behaviour, not a claim that the runner's inclusive rule is wrong as a checkers rule. Exact reproduction requires an explicit decision about that boundary. The extra search can change a move or later process state; there is no fixed score direction.

## 1. CheckerBoard's own behaviour

### A. Game and ballot order

Sources:

- CheckerBoard.c, [game0_to_ballot0](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L3928), AutoThreadFunc ENGINEMATCH at :4188.
- CBstructs.h, [emstats_t::engine1_plays_black/get_enginenum](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CBstructs.h#L142).
- utility.c, [get_3move_index](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/utility.c#L600), three_move_table at :23.

The ballot index is integer game_index/2, modulo deck size. Each ballot is played twice consecutively. The primary engine is Black in the odd-numbered first game and White in the even-numbered second game. After a three-move ballot White is to move, so the secondary engine makes the first searched move of game 1.

Selected ballots run in increasing ACF table order. The match controller does not call the random getopening function used for a random individual three-move game.

To select 156 ballots, enable cross-board and mail-play, disable barred. Public defaults select cross-board only: 144, not 156. PDN-file start positions are also supported, in file order, using each entry's final position.

Ran: all 156 included moves/order exactly match our embedded acf156 and the concatenation of Book Lab's committed acf156_h1.txt/h2.txt. The production acf156_all.txt path is not committed; I did not read that protected path. [checkerboard-156-order.tsv](tables/checkerboard-156-order.tsv) lists the full source-derived order.

For fresh DLLs in an uninterrupted full match, both real-game ages equal the 1-based game number minus one, excluding warm-up:

| ACF | Deck position | Primary Black game / age | Primary White game / age |
|---:|---:|---|---|
| 16 | 14 | 27 / 26 | 28 / 27 |
| 43 | 39 | 77 / 76 | 78 / 77 |
| 71 | 63 | 125 / 124 | 126 / 125 |
| 134 | 121 | 241 / 240 | 242 / 241 |
| 163 | 147 | 293 / 292 | 294 / 293 |

These are predictions, not measurements of missing c1 PDNs. Prior GUI use, reload/restart or resume can change initial state.

### B. DLL lifetime

Source: CheckerBoard.c, [load_engine/loadengines](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L5517), initengines at :5688, setcurrentengine at :4619, shutdown at :1626.

Both DLLs load into the GUI process at initialization/selection. Moves select stored function pointers. Neither next-game nor next-move code loads/frees a DLL. FreeLibrary occurs when replacing an unused module or closing the application.

They remain loaded through a match and can survive across multiple matches. New-match statistics reset does not reset private DLL statics. Selecting the same module can reuse its handle; two slots naming the same DLL are not guaranteed independent instances. Regicide versus another opponent does not have that self-play issue.

Public GUI source cannot establish what private state a closed engine retains, whether it writes learning files, or what its reset flag clears.

### C. Start signals, warm-up and history

Sources: CheckerBoard.c, [quick_search_both_engines](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L3795), AutoThreadFunc :4204/:4375, SearchThreadFunc :3369, newgame :5332; utility.c, [send_game_history](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/utility.c#L773).

Match start/resume queries names and performs one warm-up per engine. Each gets the standard Black-to-move board, a set gamehist command for it, then getmove(maxtime=0.1, info=0, moreinfo=0), playnow initially zero. The returned board is discarded for the next setup.

For a real game, movecount resets. The first two real search attempts set CB_RESET_MOVES: each side gets it on its first getmove, not subsequent calls. The ballot itself is set up by the GUI without searching.

Before every getmove, warm-ups included, send_game_history sends the FEN at the beginning of the reversible stretch plus king non-capture from-to moves. Man moves, captures/promotions end the preceding stretch. A fresh game/conversion gives the current FEN alone. This supplies exact history independently of an engine's own stored move list.

newgame is a GUI function, not an enginecommand. I found no automatic newgame, hash-clear, ponder or learning-reset command in this path. The reset bit delegates clearing policy to the DLL.

Other queries occur: setcurrentengine asks get book and probes get movelist B:W1:B; supported move-list engines are queried for the actual position too. get gametype/name calls occur around setup/notation. They are part of the command stream, although not searches. The API reference describes reset/gamehist; the source establishes call order.

Our src/cb_api.cpp:getmove calls Engine::new_game on reset. Current src/search.cpp:Engine::new_game clears TT and worker histories, then supplied gamehist determines current history. This does not prove another engine clears those things.

### D. Time per move, bank and increment

Sources: CheckerBoard.c, [reset_game_clocks](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L347), maxtime_for_non_incremental_tc :3237, SearchThreadFunc :3375–3468; [help.htm, engine matches](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/help.htm#L464).

At the fixed 0.5-second level:

1. Each colour's bank starts at zero per game.
2. Before a search add M=0.5 to that colour's bank.
3. With R the bank after addition, maxtime is 0.4M if R<0.4M; M+(R-M)/2.5 if R>M; otherwise R.
4. Subtract elapsed time around getmove only. History, GUI drawing, animation and preparation are outside this debit.
5. Banks are separate for the colours and reset next game.

First maxtime is 0.5. After using 0.1, that colour's next bank is 0.9 and maxtime 0.66; after using almost zero it gets about 0.7. Deficits are allowed, with a 0.2-second floor at this level. No bank carries between games.

The fixed match branch does not set CB_EXACT_TIME, even if the menu option is selected. It does not send a Fischer increment or packed bank; moreinfo is zero, with info only carrying reset when applicable. The engine sees adjusted maxtime.

Explicit incremental mode is different: each side starts at initial_time+increment, receives packed increment/remaining in info/moreinfo and a legacy maxtime fallback, then adds increment minus elapsed after each call. Handicap can scale the secondary engine's allocation. The Book Lab fixed-level runs do not use that mode.

The inspected match/timer path does not automatically stop or forfeit an engine for overrunning its bank. Negative incremental balances are logged, not scored as losses. The API/help describes maxtime as an average expectation. Clock resolution, actual scheduling and DLL time policy remain runtime matters.

Ran: the reference allocator and our helper agree exactly at eleven balances including deficit, floor, equality and surplus cases.

### E. Endings and adjudication

Sources: CheckerBoard.c, [SearchThreadFunc no-move handling](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L3309), claims/draws :3573, changeCBstate :3768, AutoThreadFunc :4230, [update_match_stats](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L4413); utility.c:detect_nonconversion_draws.

- No legal move at turn start is a loss before another getmove.
- After a legal move, the GUI tests repetition and non-conversion. Conversion is a capture, man move or changed piece type. The draw threshold is 79 consecutive reversible plies: 40 turns by one side, 39 by the other. Repetition includes side to move, with finding 2's boundary.
- Draw claims are accepted; win/loss claims require early adjudication. Neither a second engine nor an independent database must agree.
- Early adjudication defaults on in registry.c, but is a dialog setting.
- The long-game limit is maxmovecount=200 when early adjudication is on, 300 otherwise. The controller tests strictly greater than the limit and increments before each real search. A continuing game therefore reaches 201/301 search attempts after its ballot, then is recorded **unknown**, PDN result *, not drawn. An attempt producing no valid move can also consume this counter.
- There is no independent endgame-table referee in this path. Engine return values can convey table results.
- Draw detection follows the move, before the next turn's no-move test. Our runner gives an immediately terminal successor priority over draw counters, another difference at that boundary.

Ran the extracted update_match_stats at movecount 201/limit 200 and 301/limit 300: both increment unknowns, not draws, and produce *. Complete long GUI games were not run.

For an invalid English returned board, the GUI restores the original board and appends no move. Our host reports an illegal move, which the runner scores as a loss. Our host also accepts a valid CBmove fallback with an unchanged board; the GUI's English path identifies the move from a changed board. These are error/legacy-engine differences, not established occurrences in the legally replayed experiment records.

## 2. Our emulation and Book Lab settings

Read: experiments/booklab/scripts/bl_run_w.sh and mo3_run.sh at the stated head. The c1 recipe is:

~~~text
runner match-1c6e from master 1c6e997; byte-compared copy of its cbhost
Regicide DLL bl-1c6e997
--allow-rejected-options --pairs-together
--openings acf156_all.txt --movetime 0.5 -c 1 --pdn-comments
~~~

Processes are kept. Bank mode is the default. The scripts do not pass --fresh-engines, --accept-draw-claims, --max-plies, --tc, --no-time-forfeit or a claim-adjudication setting. Outer read-back pre-flight hosts are separate processes, not the processes later used for the match.

Regicide gets hashsize 128, threads 1, then book 3. Opponents get the specified dbpath/enable/cache/piece limit and hashsize 128; KingsRow gets searchthreads 1; all get book 3. --allow-rejected-options can leave other rejected settings at engine defaults. The pre-flight requires book read-back, not proof of every option's semantics. A GUI comparison needs matching actual engine settings and files; the GUI does not automatically impose these Book Lab options.

| Aspect | CheckerBoard source | Current runner/Book Lab | Effect / possible bias |
|---|---|---|---|
| Full deck | Consecutive colour-reversed pairs, primary Black first, ACF order | Same at c1 with ordered full deck | Correct order/history baseline |
| 8 single-game workers | One sequential DLL pair | Timing-dependent subsets; companions can go to different workers | Different prior state; no fixed score direction |
| 8 pair workers | One DLL pair through whole deck | Same companion processes, but each worker sees only a subset of pairs | Correct companion, different long history |
| DLL lifetime in one run | Loaded throughout | Persistent host per engine/worker unless fresh/restart | Corresponds at c1; fresh per game does not |
| State before run | DLLs can have played previous matches | Fresh processes for each runner invocation | Need a freshly loaded GUI baseline |
| Warm-up | Standard Black board, 0.1 sec, info 0 | Standard Black board, 0.05 sec, reset bit 1 | Different initialization/search state; no fixed bias |
| First real searches | Reset bit per side | Pending reset plus runner's first-side flag | Same effective DLL bit; no extra new-game enginecommand |
| Reversible history | Before every getmove | Same FEN plus suffix | Matches tested English cases |
| Extra queries | Book/movelist/gametype/name calls | Mainly startup name, options, gamehist | Opaque engine query side effects unknown |
| Fixed-time formula | Separate per-game bank per colour | Same formula | Matches; plain/exact are different modes |
| Timer | clock() around call | steady_clock around call, duration sent to four decimals | Small resolution/rounding differences, no fixed winner |
| maxtime transport | Direct double | Four-decimal protocol text | Small up/down rounding, can change millisecond thresholds |
| Slow engine | No automatic match stop/time loss | Stop at 4×maxtime+0.25; kill at stop plus max(2,maxtime) | Long thinks can be shortened or lose only in runner |
| Incremental overrun | Negative balance logged | Defaults to time forfeits | Inactive here, but differs for incremental tests |
| Draw claim | Always ends | Ignored by Book Lab | Finding 1; changes length, state, possibly result |
| Win/loss claim | Ends with early adjudication | Always ignored | Finding 1; changes result/conversion demand |
| Long game | 201/301 attempts, unknown | 1,000 engine plies, draw | More continuation, then half-points for unresolved games |
| Repetition boundary | Earliest boundary omitted | Included | Finding 2; runner can stop earlier |
| Terminal/draw priority | Draw before next no-moves check | Terminal successor wins first | Can change a boundary draw into a mover's win |
| EGDB referee | No independent referee | Hook not wired | No active difference |
| Invalid move/crash | Restore/stall or GUI-process failure | Loss, possible individual engine restart | Different score/error/later-state policy |
| Process layout | Both DLLs in one GUI process | Separate hosts | Dependency sharing, background/resource interaction differ |
| Working directory | GUI application directory before commands/search | DLL directory by default | Relative book/config/data lookup can differ |
| GUI user book | Optional, default off, bypasses DLL search | No corresponding GUI-book layer | Disable for comparison; otherwise call/clock/reset sequence differs |
| Scheduling | UI loop/thread, 20 ms controller sleeps, possible animation | Pipes, different pauses, c1 runs alongside other timed work | Potential clock/NPS/background-state effects, no fixed bias |

The runner's newgame message resets cbhost's local Game and reset_pending. It does **not** call enginecommand("newgame") in the DLL. The executed trace contains set gamehist and getmove, not a new-game DLL command.

Our host debits getmove duration, excluding history and pipe preparation, as intended by the earlier fixed ledger item. That old timing defect is not re-reported. Remaining timing differences concern precision and the independent stop policy.

The affinity script intends to put c1 KingsRow/Cake/GuiNN host pairs on logical processors 21/22/23 and other cbhosts on 0–20. I read the script, not live affinity. Logical-CPU separation alone does not establish identical load or physical-core isolation; its measured effect needs the logs.

## 3. Which run corresponds; exact matching changes

| Run | Same companion processes? | Same complete history as one fresh GUI match? | Exact overall? |
|---|---|---|---|
| 8 workers, single games | Usually no | No | No |
| 8 workers, pairs together | Yes if processes survive | No, each sees a subset | No |
| c1, pairs together, kept | Yes | Yes, with source deck order, fresh state and no restart | No: remaining differences above |
| Fresh engines per game | No continuous pair state | No | No |

At c1, the default task order also takes both legs successively; --pairs-together makes intent explicit. One long history is the decisive change, not that flag alone.

A compatibility mode would need to:

1. Define its profile: CheckerBoard revision, 156/144 deck, primary engine, early-adjudication setting, fixed/incremental level, handicap, GUI user book off, actual engine options/files, and initial DLL state.
2. Keep one pair through the complete ordered deck; do not restart at half-decks/repeats if emulating one session. Mark/stop a history-changing engine crash.
3. Match warm-up: standard Black board, 0.1 sec, no reset on that call, same history/query sequence. Keep reset on each side's first real search.
4. Accept draws and add selectable win/loss claims. --accept-draw-claims alone cannot reproduce default early adjudication.
5. Add the 200/300 attempt counter, strict boundary and unknown result. Unknown must not become a drawn half-point.
6. Explicitly choose whether to reproduce the public repetition boundary/terminal ordering or retain the runner's rules and label the difference. Source compatibility and a rules correction are distinct targets.
7. Offer the source-compatible no-automatic-timeout policy, also for incremental mode. An externally interrupted hang should not become an invented clock loss.
8. Remove avoidable serialization rounding and align working-directory/query/measurement behaviour. A shared Windows host for both DLLs would be closer for dependency/process equivalence; real-time scheduling still prevents a promise of identical search trees.

This does not invalidate board-completed games as a conversion measurement. They need that label rather than exact CheckerBoard-match equivalence.

The isolated #71 wins at age 1 do not establish White performance at full-match age 125. The repeated-ballot #163 observations do not establish White performance at age 293. Those are different histories, making the missing c1 PDNs consequential.

## 4. Remaining experiment-2 and c1 records

At b9a0f35 there are no new tagged PDNs since round 4. All 28 existing PDNs retain the decompressed SHA-256 of that manifest, so their 3,360-game process-age tables remain valid. There is no new measured row to append.

Every available E2/match-order CSV was recounted and reconciled to its committed summary. [record-availability.tsv](tables/record-availability.tsv) records counts and presence.

| Outstanding run | CSV games | Regicide W/D/L | PDN |
|---|---:|---|---|
| e2d-kr43-natural | 48 | 1/47/0 | absent |
| e2d-kr134-fresh | 24 | 0/24/0 | absent |
| e2d-kr134-kept | 24 | 2/22/0 | absent |
| e2d-kr134-natural | 48 | 9/39/0 | absent |
| e2d-kr16-fresh | 24 | 24/0/0 | absent |
| e2c-fresh-3 h1+h2 | 312 | 0/312/0 | absent |
| e2c-fresh-4 h1+h2 | 312 | 0/312/0 | absent |
| e2c-kept-4 h1+h2 | 312 | 0/312/0 | absent |
| mo1-cake | no committed CSV | unavailable | absent |
| mo1-kingsrow | no committed CSV | unavailable | absent |
| mo1-guinn | no committed CSV | unavailable | absent |

#43 fresh/kept and #71 fresh/kept/natural PDNs were already in round 4. #16 kept/natural arms have no committed records here. The wrapper-died e2c-kept-2-h2 copy is separately listed and is not an independent repeat of its later completed copy.

[new-csv-decisive-games.tsv](tables/new-csv-decisive-games.tsv) lists all 36 decisive games in the new #43/#134/#16 CSV-only arms, all Regicide White wins. Both engine-age columns explicitly say unavailable. CSVs establish opening, colour, result and termination, but not worker, age, exact moves or prior process game. A fresh run name is not a measured age tag.

I cannot yet give actual decisive-game ages for c1, verify its actual order, or inspect first varying moves in the remaining arms. The predicted order/ages cannot substitute for evidence that processes survived. Those PDN-dependent analyses remain open.

## Evidence and practical limits

- [checkerboard-156-order.tsv](tables/checkerboard-156-order.tsv): ballots, source order and predicted game numbers/ages.
- [record-availability.tsv](tables/record-availability.tsv): CSV recounts and PDN gaps.
- [new-csv-decisive-games.tsv](tables/new-csv-decisive-games.tsv): decisive CSV-only records with ages unavailable.
- [fixture-output.txt](fixture-output.txt): executed runner, detector/history/clock and limit-counter results.

The legal-cycle DLL selects the stated moves and a chosen result code, without search. Extracted-source harnesses compile actual functions from the pinned reference, with local adapters for boards, moves, FENs, UI logging and result storage. These establish those functions' behaviour, not a full Windows GUI's runtime environment.

No master/feature branch was modified. Only report text under reviews/a/round5 is committed.
