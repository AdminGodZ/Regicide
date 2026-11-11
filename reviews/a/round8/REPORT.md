# Review A — Round 8

Date: 2026-11-11. Publication: reviews/a/round8/ on t3/review-reports.

Freshly loaded Regicide defaults to built-in book mode 3, margin 0, hash 128 MB, one search worker, the built-in evaluator and the built-in avoid list. Those match Book Lab's current Regicide recipe. CheckerBoard does not overwrite them at match start. It does not discover Regicide's database through its own saved EGTB directory: Regicide looks for egdb beside its DLL. Without that directory, or an explicit successful set dbpath, the engine has no loaded tables.

Master fdd154e (PR #79, merging f3e8589) fixes both round-7 counterexamples, and its two new tests fail on e252fd0. The bench remains 7,833,556 nodes. One remaining claim boundary is detailed below: without tables, a sound mate in one is not claimed after 78 reversible plies.

All three mo2 PDN/CSV pairs arrived at 3be313a. Their 936 games independently recount to Cake 0/312/0, KingsRow 1/311/0 and GuiNN 19/293/0, with no unknown result. Against the round-7 census counterfactual there are 24 changed results; every one first differs on an opponent move.

## Sources, scope and execution

The master ledger was read first and after fetching again. Prior accepted/assigned findings are not re-reported; the new finding concerns the claim guard introduced by the proposed fix, on the newest feature commit.

| Source | Revision |
|---|---|
| origin/master | fdd154e17db15edd92dea51be3d332eccf8f2078 |
| origin/t3/claim-soundness | f3e858993dd60c7c67473af78466b58e01f7be38 |
| origin/t3/booklab | 3be313a2c7d91f5e79035b8897483a1e6f97d4ed |
| CheckerBoard reference | 32f194f840f208f0aff86a07026f788c01c819ce, version 1.78, unchanged from rounds 5 and 7 |
| Before latest fixes | e252fd0, the round-7 artifacts |

Everything ran under /home/admingod/regicide-review-a. Builds, tests, engine calls and referee fixtures used nice -n 19, at most two build jobs and one search worker. Commands were foreground and below ten minutes. Temporary files stayed in the clone. No real opponent DLL, protected directory, lock, shared database or another agent's process was used. Book Lab scripts were read from our private source archive, not executed. Their production paths quoted here are source text, not files inspected on this machine.

Executed: fresh release build of f3e8589; API and scripted-runner tests and before-code controls; bench; direct DLL option/default/claim probes; the independent draw and reply-guard comparisons; an extracted actual CheckerBoard SearchThreadFunc with controlled clock/UI adapters; actual cbhost with our no-search logging DLL. The full Windows GUI and registry were not run or modified. Source-only claims about GUI loading, dialogs, registry, file persistence and launch recipes are marked as such below. The record work independently replays legal moves and parses comments/tags; it runs no engine search.

The small database is the private round-6 two/three-piece fixture built with tests/test_egdb.hpp. The missing-table probe deliberately loads a DLL without an adjacent egdb directory. A second copy with a local egdb link to that private fixture verifies automatic module-relative discovery.

## Finding 1 — a terminal mate is not claimed at the last reversible ply

File: [src/search.cpp:807, Engine::claim](https://github.com/AdminGodZ/Regicide/blob/fdd154e17db15edd92dea51be3d332eccf8f2078/src/search.cpp#L807); the return becomes CB_UNKNOWN in getmove. The strict mate-distance clock check was introduced by 80bcfb4 and remains after f3e8589.

Concrete root: B:WK10:BK1,K6. No database is loaded (get dbpieces answers 0); HCE, book/avoid off, hash 1 MB, one worker, playnow 0, reset flag 1, maxtime 0.01. Supply the complete legal 78-ply king history in [terminal-history78.txt](fixtures/histories/terminal-history78.txt).

Observed:

| Root reversible count | Move | Search score | getmove return | Independent referee result |
|---|---|---|---|---|
| 0 | 6x15 | 29999 | CB_WIN, 1 | Black win |
| 77 | 6x15 | 29999 | CB_WIN, 1 | Black win |
| 78 | 6x15 | 29999 | CB_UNKNOWN, 3 | Black win |

All 78 preparatory moves are legal quiet king moves, all positions are distinct, and no rule draw has happened before the tested call. The forced 6x15 captures White's last piece, resets the reversible count to zero and leaves no opponent move. The search completes mate in one in two nodes through getmove. The preceding master DLL claims it at all three counts.

Engine::claim computes 78 + (30000 - 29999) = 79 and requires it to be strictly below 79, as if the mating move were another non-converting move and a draw took priority over its terminal win. The new reply check is not the cause: there are no opponent replies here. This is a sound short win the current guard no longer claims. The guard should preserve the terminal-win priority already used by draws_at_once/the referee, after excluding a root that was already drawn.

A missing win claim normally lets the GUI find the win on the next attempt. It matters at an attempt limit: if this is the last allowed attempt, an accepted win claim ends it there, whereas finding the opponent without a move on the next attempt produces unknown under CheckerBoard's update_match_stats. That impact is conditional on the limit boundary; the board itself is won in every tested call.

Evidence: [terminal-clock-api.tsv](tables/terminal-clock-api.tsv), [terminal-clock-before.tsv](tables/terminal-clock-before.tsv), [api_terminal_clock.py](fixtures/api_terminal_clock.py), [terminal_clock.cpp](fixtures/terminal_clock.cpp). The last file constructs and verifies the legal distinct history, then reproduces the same None claim through the core with tables disabled.

## Task 1 — every GUI-generated enginecommand and its effect on Regicide

This inventory covers the GUI's actual C/C++ call sites, including the direct enginecommand1/2 calls. The example DLL implementations in source/simplech.c and source/dama.c are engines, not additional GUI senders. There is no other command export acquired under a hidden alias: load_engine obtains enginecommand, enginename, getmove and islegal, then selects those pointers.

Load itself only uses LoadLibrary/GetProcAddress: [CheckerBoard.c load_engine :5518–5547](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L5518). loadengines calls setcurrentengine(1) at :5675. In normal state that asks the name, then book state and move-list capability; gametype is queried when checking whether the board needs resetting. There is no automatic set hashsize, set book, database-path, thread, init, newgame or hash-clear enginecommand at load or match start.

The Regicide reply column is executed for the queries in [options-no-auto-db.tsv](tables/options-no-auto-db.tsv); setters and their effects are traced in src/cb_api.cpp enginecommand setters. All commands first enter state(), so the first query also performs the one-time engine/book/default-database setup. Later read-only queries do not clear hash/history or change an option.

### Commands sent during ordinary selection and matches

C = CheckerBoard.c, U = utility.c in the pinned reference.

| Exact command | GUI function and line | When sent | Our reply/effect |
|---|---|---|---|
| name | C enginename :4698/:4710; AutoThreadFunc :4206/:4208 | Window/player names, selection, log naming, match start; can recur during play | 1, Regicide 0.2; no option change after setup |
| get book | C setcurrentengine :4644; WindowFunc :1425 | Every selection of the current engine, including per-search selection; after options dialog closes | 1, current mode, initially 3; only updates GUI book-state display |
| get movelist B:W1:B | C setcurrentengine :4648 | Capability probe when current engine is selected | 0, ?; GUI uses its own English-checkers generator |
| get movelist <current FEN> | C get_movelist_from_engine :5960–5962 | Only if capability probe succeeded | Not reached for our DLL; unsupported |
| get gametype | C gametype :4664–4666 | Board setup/render/search/type checks, including after load | 1, 21 (English checkers); no state change |
| set gamehist <FEN> [quiet king moves...] | U send_game_history :773–803; C SearchThreadFunc :3412 | Immediately before each actual getmove; also before both match warm-ups at :3807/:3813 | 1, empty reply; replaces pending exact history. It is consumed by the next getmove, then cleared. No setting changes |

Match start also calls quick_search_both_engines, C :3798–3814: each DLL gets one getmove on the starting board with maxtime 0.1, info 0, moreinfo 0, preceded by set gamehist. These are not enginecommand new-game messages. Each real game's first search for each side later has CB_RESET_MOVES set by AutoThreadFunc :4378–4380.

At unload there is no enginecommand quit, save, stop or deinit. loadengines calls FreeLibrary for a replaced unused module at C :5609/:5623; WindowFunc shutdown aborts a search then calls FreeLibrary at :1626–1629. Regicide's static database mappings are released on ordinary library unload; there is no settings-save callback.

### Commands generated by option dialogs

D = [dialogs.c getengineoptions :941](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/dialogs.c#L941), [get_more_engine_options :1058](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/dialogs.c#L1058). These queries are automatic when a dialog is opened or its selected engine changes. Setters are conditional on a user changing a field and applying/OK; they are not sent merely to start a match.

| Query / conditional setter | Function/line | Our answer and state effect |
|---|---|---|
| get hashsize / set hashsize N | getengineoptions :948 / setengineoptions :910–911 | Get 1, initially 128 MB. Set 1, requested size clamped to at least 1; resizes/clears TT. Allocation failure keeps old size and returns 0 |
| get dbmbytes / set dbmbytes N | :963 / :915–916 | Get unsupported, 0 with ?; cache control disabled. Set is supported if sent directly: updates pre-fault budget, initially 4096 MB, calls egdb::set_cache; no table identity/piece-limit change |
| get book / set book N | :977 / :922–923 | Get/set supported; set clamps 0–3 and selects built-in book/margin as below |
| get allscores / set allscores N | :1025 / :928–929 | Both unsupported, 0 with ?; disabled control; no all-scores search mode |
| get solve / set solve N | :1037 / :934–935 | Both unsupported, 0 with ?; disabled control |
| get dbpath / set dbpath PATH | getengineoptions :1050; get_more_engine_options :1067 / set_more_engine_options :1283–1284 | Get 1, loaded path or empty. Enables More Options/path controls even with no DB. Set loads the first successful directory and sets complete WLD piece coverage; same directory accepted, different already-loaded directory refused |
| get enable_wld / set enable_wld N | get_more_engine_options :1095 / set_more_engine_options :1301–1302 | Unsupported, 0 with ?; control disabled. Loaded table use is automatic |
| get max_dbpieces / set max_dbpieces N | :1108 / :1277–1278 | Unsupported, 0 with ?; limit control disabled. Our get dbpieces reports coverage, but is not the GUI's command and is not a setter |
| get enable_mtc / set enable_mtc N | :1129 / :1307–1308 | Unsupported, 0 with ?; control disabled. MTC available from the loaded Regicide table directory is used automatically |
| get mtcpath / set mtcpath PATH | :1142 / :1289–1290 | Unsupported, 0 with ?; control disabled. Our WLD and MTC files share the one directory |
| get bookfile / set bookfile PATH | :1170 / :1295–1296 | Unsupported, 0 with ?; control disabled. Our interface uses the embedded book, not a GUI-selected external book file |
| get searchthreads / set searchthreads N | :1190 / :1313–1314 | Unsupported, 0 with ?; thread control disabled. Our spelling is get/set threads; default one |
| get cpus | get_more_engine_options :1195 | Conditional on get searchthreads succeeding, so not sent for our DLL; unsupported if sent |
| set check_wld_dir PATH | DialogFuncMoreOptions :697–698 | Only Check WLD button; unsupported, 0 with ? |
| set check_mtc_dir PATH | DialogFuncMoreOptions :704–705 | Only Check auxiliary DB button; unsupported, 0 with ? |

The disabled cache list has no selection, and getoptionsfromdialog checks the selection before reading it (D :869–873); pressing OK unchanged does not turn an unsupported get dbmbytes into a cache-size assignment.

The remaining fixed GUI-generated command strings are explicit user actions, not automatic match setup: WindowFunc :1432–1433 sends staticevaluation <FEN> (unsupported); :1441 sends about (1, engine information); :1447 sends help (0, empty); :1522–1523 sends set book N when the toolbar cycles book mode. DialogFuncEnginecommand :579 forwards arbitrary text the user typed. These sites introduce no hidden setting at a match boundary.

### Book mapping and database discovery

Our [src/cb_api.cpp state/setup :65–142](https://github.com/AdminGodZ/Regicide/blob/fdd154e17db15edd92dea51be3d332eccf8f2078/src/cb_api.cpp#L65) starts at mode 3. Engine::set_book's default margin is 0 (src/search.hpp :145); state() calls it without a different margin. The mode reported to the GUI and the actual margin therefore agree before any explicit setter.

| GUI book value | GUI meaning | Regicide state |
|---|---|---|
| 0 | Off | Book pointer cleared; the separate built-in avoid list remains enabled |
| 1 | All kinds | Built-in book, margin 60 |
| 2 | Good moves | Built-in book, margin 10 |
| 3 | Best moves | Built-in book, margin 0 |

set book does not load a different file. At margin 0 the mode is deterministic, as settled in the ledger/round 2. Defaults also retain built-in n8q evaluation and built-in r11d avoid entries when using the current DLL; choose the same DLL/embedded weights as the compared Book Lab run.

Regicide library_dir uses the DLL's own filename, not CWD or the GUI registry. On first enginecommand/getmove it tries <DLL directory>/egdb with a 4096 MB pre-fault budget; successful load sets the engine's WLD piece limit to egdb::max_pieces(). Complete MTC coverage comes from the same files. If nothing loads, get dbpath is empty and get dbpieces is 0. No path is searched under CheckerBoard's default db directory or by reading KingsRow's registry. Regicide-format files are required; the opponent's KingsRow WLD directory is a different input.

Executed controls: the DLL alone answers hash 128, book 3, threads 1, dbpieces 0. A DLL copy with its adjacent egdb link to our private fixture automatically answers that module-relative path and dbpieces 3, with no set dbpath. A direct set dbpath after the failed default attempt loads that private fixture and answers 3. [options-auto-db.tsv](tables/options-auto-db.tsv) records the automatic case.

Book Lab's [bl_build_dll.sh :39–46](https://github.com/AdminGodZ/Regicide/blob/3be313a2c7d91f5e79035b8897483a1e6f97d4ed/experiments/booklab/scripts/bl_build_dll.sh#L39) creates an egdb directory junction beside each installed Regicide DLL, pointing to its Regicide 2–8-piece copy. That is how the recipe supplies our tables; bl_run_w.sh does not send set dbpath to Regicide. I verified the committed deployment recipe, not the protected live junction.

### Persistence between GUI sessions

Source-only: [registry.c :22–50/:53–205](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/registry.c#L22) saves/loads the CBoptions structure under HKCU\Software\Martin Fierz\CheckerBoard64\1.78 (32-bit: CheckerBoard\1.78), value options structure. It includes selected DLL filenames, time level, incremental/exact/early-adjudication/handicap flags, opening filters, userbook enable, GUI paths/window settings. It does not contain engine hash size, engine book level, engine search threads or the engine's DB path/cache options. ENGINE_OPTIONS and MORE_ENGINE_OPTIONS are dialog-local/static copies, not serialized engine settings.

A fresh GUI defaults to 2 seconds per move, incremental off, exact off, early adjudication on, handicap off, userbook off, cross-board ballots on and mail-play/barred off (registry.c :152–182). Selecting our primary DLL does not import an earlier engine's options.

The GUI separately reads Documents/Martin Fierz/CheckerBoard/userbook.bin at C init :3006–3010, writes it when editing/removing entries (addmovetouserbook :2256 and WindowFunc :1341), and can bypass getmove with its move when userbook is enabled (SearchThreadFunc :3335–3349). This is separate from our embedded book. Match stats/progress/PDN/log files are written under its match directory and support GUI match resumption; that does not restore a fresh DLL's former internal process state.

Regicide's CheckerBoard API does not write a registry key or settings/learning/book/hash file. It loads external evaluator/avoid files only on explicit API commands; database runtime mappings are read-only (src/egdb/platform.cpp :26–34). The pre-fault budget touches mapped pages; it is not a separate 4 GB allocated hash/cache. src/egdb/store.cpp's writer is for generation, not called by engine init/probing.

Executed: after setting hash 64/book 2 and later book 3 in one process, a fresh process again answers hash 128/book 3/threads 1/no database. [options-fresh-again.tsv](tables/options-fresh-again.tsv) confirms the lack of engine-setting persistence. Within one loaded DLL options remain until changed/unloaded; new_game clears search/hash histories, not those option fields. That in-memory lifetime must not be confused with restarting the program.

### What our runner/host and Book Lab send

[tools/match/engine_process.cpp Engine::launch :138–189](https://github.com/AdminGodZ/Regicide/blob/fdd154e17db15edd92dea51be3d332eccf8f2078/tools/match/engine_process.cpp#L138) waits for host ready, sends each configured option as cmd <text>, then does a warm-up: newgame, position startpos, go maxtime 0.05 reset. Those are host-protocol messages; only cmd contents reach enginecommand. A rejected configured command aborts launch unless --allow-rejected-options was supplied.

[cbhost main :397–401/run :204–280](https://github.com/AdminGodZ/Regicide/blob/fdd154e17db15edd92dea51be3d332eccf8f2078/tools/cbhost/cbhost.cpp#L397) sends the DLL name query at load and forwards cmd text. Its newgame only resets host board/reset_pending and replies ok; it does not forward enginecommand newgame. Before every getmove, by default, it sends set gamehist <FEN> <moves> from the last conversion. It then supplies getmove's reset bit on each side's first actual search. Quit exits the host; it sends no DLL quit/save command. main :407–408 deliberately uses _Exit rather than unloading DLL/static destructors.

No get protocolversion, get gametype, get movelist, get hashsize, get allscores or get searchthreads is sent automatically by our host. id protocol cbhost 1 describes the pipe protocol, not an enginecommand query or the DLL's API-version answer.

Current [bl_run_w.sh :38–68](https://github.com/AdminGodZ/Regicide/blob/3be313a2c7d91f5e79035b8897483a1e6f97d4ed/experiments/booklab/scripts/bl_run_w.sh#L38) supplies these exact option strings:

| Engine | Enginecommand option sequence |
|---|---|
| Regicide | set hashsize 128; set threads 1; then set book 3 |
| Cake or GuiNN | set dbpath C:\regicide-match\db\kingsrow_wld8; set enable_wld 1; set dbmbytes 512; set max_dbpieces 8; set hashsize 128; then set book 3 |
| KingsRow | Same opponent sequence, with set searchthreads 1 before set book 3 |
| Regicide with EXTRA_A | Extra command after hash/threads and before book; followed by get evalfile. mo2's committed invocation has no EXTRA_A |

Before real launch, its separate readback-only book_probe.py process sends the same options, set book 3, get book; the script requires the latter to return 3 and aborts otherwise. That probe process does no search and is not reused as a match process. The actual runner receives setters, without a further get book in its own process. All three setters for Regicide are supported in the direct probe.

The mo2 recipe explicitly uses --allow-rejected-options. For opponents, the table is a list of requested commands, not proof every DLL accepts every setting; the book mode is the one enforced by the pre-flight read-back. I did not load any opponent DLL to test its optional commands, and the committed run records contain no complete command/reply log certifying the other options. The same accepted/refused settings must be reproduced on the GUI side when comparing an opponent.

[mo2_run.sh](https://github.com/AdminGodZ/Regicide/blob/3be313a2c7d91f5e79035b8897483a1e6f97d4ed/experiments/booklab/scripts/mo2_run.sh) uses the full deck, 0.5 seconds, concurrency 1, pairs together, kept processes, --cb-claims --cb-move-limit 200, master 8de39d0 runner/host/DLL. It launches a private copy of the host for each opponent's run. cl_chain.sh uses the same engine settings but eight workers, halves and no action on claims. Neither launcher was run in this review.

| Difference | Real GUI | Runner/host recipe | Consequence for our engine / matching |
|---|---|---|---|
| Book mode at load | Reads mode, does not set it | Explicit set book 3 + separate read-back | Fresh Regicide is mode 3 in both; a previously changed still-loaded GUI DLL needs resetting |
| Hash | DLL default 128; dialog may change it | Explicit 128 | Fresh default agrees; reapply/check after earlier manual changes |
| Workers | DLL default 1; searchthreads UI unsupported | Explicit set threads 1 | Fresh default agrees; manual get/set threads is the supported spelling |
| Regicide DB | Module-relative egdb or explicit path | Installed adjacent junction, automatic load | Equal only with the same complete Regicide tables supplied; GUI EGTBdirectory does not configure it |
| Cache budget | Regicide default 4096; GUI get dbmbytes unsupported | Same default for Regicide; 512 is for opponents | Our defaults agree; do not copy the opponent's 512 MB line onto Regicide when claiming an identical setup |
| Evaluator/avoid | Built-in defaults; no standard GUI controls | Built-in unless EXTRA_A changes them | Use the identical embedded DLL/net/book and any explicit run override |
| Automatic queries | name, gametype, book, movelist probe | name only, plus explicit recipe/readback | Queries do not change our play after initialization |
| Working directory | CheckerBoard executable directory before calls | DLL directory unless --cwd | Our module-relative defaults are stable; an opponent's relative-file behaviour may differ |
| Warm-up | One 0.1 s starting-board call per DLL, info 0 | One 0.05 s starting-board call, info 1 | Known accepted profile difference; may affect an opponent's retained process state |
| Per-game reset/history | No new-game command; reset on each side's first call, reversible gamehist every call | Host newgame not forwarded; same reset/history convention | Our API receives the intended new-game/history signals |
| Unload | FreeLibrary, static cleanup | _Exit, OS cleanup | Neither saves our engine options/learning to disk |
| Time arguments | Bank-derived, unrounded double, no exact bit in match mode | Same formula; maxtime serialized to four decimals | Same policy, at most the serialization rounding difference in the offered value |
| GUI userbook | Can bypass getmove if enabled | No GUI userbook layer | Leave GUI userbook off |
| Opening default | Cross-board only, 144 ballots | Production 156 includes mail-play | Enable cross-board and mail-play, barred off, to match the committed 156 deck |

The warm-up, repetition-window boundary and timeout policy remain the accepted differences in the ledger. This table does not re-report them as findings.

### Settings to reproduce Book Lab's Regicide configuration in a fresh GUI

1. Select the same 64-bit Regicide DLL/embedded r11d book/n8q net as primary engine. A fresh DLL already has book 3, hash 128, threads 1, built-in evaluator and avoid list enabled. The GUI defaults do not force book mode 1 or 0.
2. Deploy the matching Regicide egdb folder beside that DLL, or use More Options' WLD path / an explicit set dbpath before any different directory has loaded. Verify with get dbpath and get dbpieces (the intended full bundle answers 8; our small fixture answers 3). A selected GUI db folder, a KingsRow-format directory or an empty reply is not equivalent. Once a directory loaded successfully it cannot be exchanged in that instance: fully unload the DLL or restart, rather than assuming a new-game reset will change it.
3. If continuing an already-used GUI/DLL, set/check hash 128 and book 3. set threads 1 uses the manual engine-command dialog; the GUI's searchthreads control is disabled for Regicide. These settings are not restored from an engine registry entry on the next program launch. With a fresh load no extra setter is needed for the matching defaults.
4. Set the GUI time level to 0.5 s/move (fresh GUI default is 2 s), non-incremental mode, handicap off. Exact-time need not be enabled; that checkbox is ignored by the engine-match branch. Use early adjudication on, matching --cb-claims and its 200-attempt limit.
5. Disable the GUI userbook; select cross-board + mail-play, barred off, repeat 1, fresh match rather than a resumed statistics file. That supplies the same 156 ballots in order and consecutive reversed-colour games; primary is Black first. Match the opponent's book/hash/database/thread settings to the accepted recipe options as well.

This matches the requested settings and single-pair process order; it does not eliminate the accepted warm-up, timing/rounding, repetition-boundary and timeout differences. No setting in the fresh GUI alone installs the missing Regicide table files.

### Exact time arguments for the first calls

Sources: [CheckerBoard.c reset_game_clocks :347–358](https://github.com/eygilbert/CheckerBoard/blob/32f194f840f208f0aff86a07026f788c01c819ce/CheckerBoard.c#L347), SearchThreadFunc :3371–3418, AutoThreadFunc :4378–4380; tools/match/main.cpp :227–240; cbhost.cpp :254–270/:297–298.

Let F(r) be the already-verified non-incremental bank function at a 0.5 s level. It returns 0.2 if r < 0.2, r if 0.2 <= r <= 0.5, otherwise 0.5 + (r - 0.5)/2.5. Each colour starts at zero, receives 0.5 before its call and is charged its own getmove elapsed time.

After a three-move ballot the first searched side is White. For elapsed eW1 on White's first call, the first three global calls are exactly:

| Global call | Side | GUI maxtime | info | moreinfo | cbhost in normal match mode |
|---|---|---|---|---|---|
| 1 | White | 0.5 | 1, reset only | 0 | Same, 0.5000 serialized |
| 2 | Black | 0.5 | 1, reset only | 0 | Same, 0.5000 serialized |
| 3 | White | F(1.0 - eW1) | 0 | 0 | Same formula using charged host time, rounded to four decimals |

For any one engine's first three calls, offers are 0.5, F(1.0-e1), F(1.5-e1-e2); flags 1,0,0, with moreinfo 0 throughout. They are not necessarily 0.5 on every move. A near-instant book return makes the next offer about 0.7; a 0.1 s first call makes it 0.66. No incremental-time fields are encoded.

I compiled the complete pinned SearchThreadFunc with a controlled 0.1 s elapsed time per scripted call. Its first six global offers are 0.5, 0.5, 0.66, 0.66, 0.82, 0.82; info 1,1,0,0,0,0, moreinfo always 0. I repeated with cboptions.exact_time false and true: the output is identical, because the exact-bit code is in the non-match branch. [gui-time-arguments.tsv](tables/gui-time-arguments.tsv) is the raw table.

A separate actual cbhost run forwarded supplied offers 0.5,0.66,0.82 to our logging DLL with info 1,0,0 and moreinfo 0, [host-arguments.log](evidence/host-arguments.log). Our own DLL interprets this as target_ms from maxtime, not exact fixed movetime. The runner's host-time report is four-decimal seconds as well; GUI clock resolution and runtime scheduling need not produce identical elapsed samples. The controlled numbers establish the argument policy, not actual elapsed times of a particular match.

## Task 2 — final claim fixes and boundary verification

Fresh build/test source is f3e8589, after e252fd0. After master fdd154e merged PR #79, all 141 src/tools/tests/CMake source and weight files were byte-compared with that build and are identical; the fixtures, bench and guard comparisons were rerun after fetching the merge. The binary thus tests current master without changing to any source branch. Both round-7 findings are fixed:

| Case | f3e8589 observation | Before-code evidence |
|---|---|---|
| B:WK3:BK1,K2 with the 6-ply cycle history; 1-5 permits White's third-occurrence reply 3-8 | Still plays 1-5, 19994, but returns CB_UNKNOWN 3 | e252fd0 returns CB_WIN 1 |
| B:W10:B6, 6x15 with win claim, --cb-claims --cb-move-limit 1 | Both mock claimants' games 1-0 within one attempt | e252fd0 recorded both * |
| Public asymmetric terminal fixture, only A claims | A-Black leg wins at attempt 1; B-Black unknown leg reaches missing move at attempt 2 and stays * | New runner movelimit test fails on e252fd0 |
| Updated API regression for opponent's third-occurrence reply | Passes | New cb_api_test fails on e252fd0 |

The original round-6 controls still pass: late MTC win after 77 reversible plies is not claimed; an already third-occurrence root is not claimed won; the played move completing third occurrence returns CB_DRAW; second occurrence does not; count 78 + a quiet move returns CB_DRAW at 79, while count 77 + a quiet move does not; N+1 draw claims are unscored; the two claim flags together are refused. Ordinary B:W10:B6 and its White counterpart still return CB_WIN, as does the fresh three-piece MTC win B:WK3:BK1,K2. The searched-table half-window tests at 4/37/38 king plies also retain their intended boundary.

Executed tests: cb_api_test, and runner claims, cbclaims, movelimit, claimflags, pairs, restart; all pass. Eight scripted default-option games have identical before/after result fields and claim metadata. Full results: [api-cases-after.tsv](tables/api-cases-after.tsv), [claim-api-test.log](evidence/claim-api-test.log), [runner-cases.log](evidence/runner-cases.log).

Bench: 7,833,556 nodes. All 17 individual position/move/score/node rows equal the round-7/master baseline, not just the total.

The refactored rule_draw helper now receives the full history before its tested position, including root/child as needed. The immediate-draw predicate still matches the independent referee for 11,171 roots and 54,516 legal candidate moves: 169 draw returns, 25,778 converting moves and 261 terminal moves, zero disagreements.

For the new opponent-reply guard, I additionally supplied synthetic mate scores to those same legal candidate moves and enumerated every legal opponent reply independently with tools/common's board/referee. This tests the guard's history/window arithmetic, not whether those synthetic scores are real wins. There are 578 candidates with an available rule-drawing reply, and none is returned as a win claim; 53,860 synthetic candidates survive the full claim guards. The corpus includes the long legal histories, seeded walks, conversions and terminal replies. [reply-guard.log](evidence/reply-guard.log), [reply_guard.cpp](fixtures/reply_guard.cpp). These are bounded tests, not a proof for every position.

The actual round-7 table example also passes through getmove with its real score, as shown above. I found no remaining win claim with a rule draw available on the opponent's next reply in these checks. Finding 1 supplies the requested opposite check: a real sound short win withheld at the no-progress boundary with no tables. The accepted no-distance half-window policy is unchanged; the stopped-call fallback assigned to reviewer B is not re-reviewed here.

## Task 3 — independent mo2 replay and comparison

The final record fetch brought all three runs at Book Lab 3be313a, tests/mo2-{cake,kingsrow,guinn}. Each has 312 PDN games and matching unique CSV game identifiers, exit status 0. I replayed all 46,260 new moves with the separate tools/common board/referee and also replayed the six unchanged census halves, 148,054 moves in total. The census hashes match the round-7 sources. No illegal move was found. Claims parsed from comments agree with ClaimsA/B and FirstClaim/CSV; actual termination handling and mover-perspective results agree with the records. [mo2-sources.tsv](tables/mo2-sources.tsv) contains the decompressed hashes.

All results below are Regicide's scored results. These runs use the original 8de39d0 DLL/runner, not the later claim-soundness fixes.

| Run | W | D | L | Unknown | How the games ended |
|---|---|---|---|---|---|
| mo2-cake | 0 | 312 | 0 | 0 | 312 Cake draw claims |
| mo2-kingsrow | 1 | 311 | 0 | 0 | 306 KingsRow draw claims; 5 referee repetitions; 1 Regicide win claim |
| mo2-guinn | 19 | 293 | 0 | 0 | 292 GuiNN draw claims; 1 referee repetition; 12 Regicide win claims; 7 GuiNN loss concessions |

There are 930 claim endings, six board-rule draws, no move-limit ending and no terminal board win. Each accepted claim is on the final recorded move, after an otherwise nonterminal/non-rule-drawn board. The longest record has 199 total plies (196 engine calls), so none reaches the 200-attempt boundary or the earlier runner's N+1 bug. The six referee draws independently satisfy threefold repetition. No result is replaced with a presumed loss/draw on a merely high/low evaluation.

Consequently the 20 wins here are claimed/conceded scores, not board wins played to completion; replay alone does not prove the claimed positions won. This differs from the original census, where play continued to a board result. The totals and termination counts agree with Book Lab's committed summaries, after the independent replay.

Every mo2 game has Worker 0 and EngineGamesA = EngineGamesB = game number - 1. Thus there is one long recorded process history per side, with no counter reset/restart in these runs; warm-up calls are excluded from the game-age tags. All decisive games are listed below. There are no unknown games to list. Ages are read and checked, not predicted from scheduling.

| Opponent | Game / Round | ACF | Our colour | Age A/B | Ending | Final ply |
|---|---|---|---|---|---|---|
| kingsrow | 78 / 39.2 | 43 | W | 77/77 | our win claim | 128 |
| guinn | 10 / 5.2 | 6 | W | 9/9 | our win claim | 40 |
| guinn | 16 / 8.2 | 9 | W | 15/15 | our win claim | 76 |
| guinn | 28 / 14.2 | 16 | W | 27/27 | our win claim | 78 |
| guinn | 42 / 21.2 | 23 | W | 41/41 | guinn loss concession | 59 |
| guinn | 62 / 31.2 | 34 | W | 61/61 | guinn loss concession | 43 |
| guinn | 72 / 36.2 | 40 | W | 71/71 | our win claim | 82 |
| guinn | 78 / 39.2 | 43 | W | 77/77 | our win claim | 62 |
| guinn | 126 / 63.2 | 71 | W | 125/125 | guinn loss concession | 37 |
| guinn | 138 / 69.2 | 78 | W | 137/137 | guinn loss concession | 35 |
| guinn | 154 / 77.2 | 86 | W | 153/153 | our win claim | 78 |
| guinn | 163 / 82.1 | 92 | B | 162/162 | our win claim | 81 |
| guinn | 170 / 85.2 | 96 | W | 169/169 | our win claim | 62 |
| guinn | 180 / 90.2 | 101 | W | 179/179 | our win claim | 58 |
| guinn | 199 / 100.1 | 112 | B | 198/198 | guinn loss concession | 36 |
| guinn | 254 / 127.2 | 141 | W | 253/253 | our win claim | 40 |
| guinn | 262 / 131.2 | 146 | W | 261/261 | our win claim | 58 |
| guinn | 275 / 138.1 | 153 | B | 274/274 | guinn loss concession | 40 |
| guinn | 301 / 151.1 | 167 | B | 300/300 | our win claim | 39 |
| guinn | 312 / 156.2 | 174 | W | 311/311 | guinn loss concession | 51 |


[mo2-decisive-unknown.tsv](tables/mo2-decisive-unknown.tsv) adds exact PDN outcome, engine-call count, claim colour and final FEN; [mo2-games.tsv](tables/mo2-games.tsv) includes every game. [mo2-terminations.tsv](tables/mo2-terminations.tsv) is the full termination cross-tab.

### Census counterfactual against the new played runs

Matching is by opponent, exact three-move ballot and Regicide colour, not by local half-deck game number. There is exactly one census and one mo2 observation for each of the 936 keys. The census counterfactual stops at its first accepted claim or earlier board result, using the round-7 recorded ending ply; later played-out census moves are not attributed to a counterfactual game that already ended. All seven census games without a claim ended by ply 102, so adding the 200-attempt limit would not change the round-7 baseline either.

| Opponent | Census counterfactual W/D/L | mo2 W/D/L | W → D | D → W | Changed results |
|---|---|---|---|---|---|
| Cake | 1/311/0 | 0/312/0 | 1 | 0 | 1 |
| KingsRow | 3/309/0 | 1/311/0 | 2 | 0 | 2 |
| GuiNN | 22/290/0 | 19/293/0 | 12 | 9 | 21 |

All 24 changed-result comparisons have an actual first different opponent move before either compared record ends. The table gives the shared-prefix decision ply and the move in each run. Every result is from Regicide's perspective. C = census counterfactual, M = mo2; the first differing actor is the named opponent in every row.

| Opponent | mo2 game | ACF | Our colour | C → M | First different ply | Census move | mo2 move |
|---|---|---|---|---|---|---|---|
| cake | 257 | 143 | B | W → D | 54 | 1-6 | 18-15 |
| kingsrow | 28 | 16 | W | W → D | 41 | 9-14 | 32-28 |
| kingsrow | 242 | 134 | W | W → D | 37 | 20-24 | 11-16 |
| guinn | 8 | 5 | W | W → D | 5 | 11-15 | 1-6 |
| guinn | 39 | 22 | B | W → D | 4 | 27-23 | 26-23 |
| guinn | 42 | 23 | W | D → W | 15 | 9-13 | 4-8 |
| guinn | 46 | 25 | W | W → D | 15 | 4-8 | 9-13 |
| guinn | 48 | 27 | W | W → D | 21 | 3-7 | 12-16 |
| guinn | 55 | 31 | B | W → D | 4 | 22-18 | 27-24 |
| guinn | 72 | 40 | W | D → W | 7 | 11-15 | 9-13 |
| guinn | 112 | 64 | W | W → D | 5 | 1-6 | 11-15 |
| guinn | 114 | 65 | W | W → D | 11 | 15-19 | 8-11 |
| guinn | 126 | 71 | W | D → W | 9 | 3-7 | 12-16 |
| guinn | 132 | 74 | W | W → D | 11 | 6-10 | 11-16 |
| guinn | 138 | 78 | W | D → W | 5 | 7-10 | 6-10 |
| guinn | 154 | 86 | W | D → W | 21 | 8-12 | 16-19 |
| guinn | 160 | 90 | W | W → D | 13 | 5-9 | 11-16 |
| guinn | 163 | 92 | B | D → W | 4 | 25-22 | 24-20 |
| guinn | 170 | 96 | W | D → W | 15 | 9-13 | 1-5 |
| guinn | 236 | 131 | W | W → D | 11 | 10-15 | 8-11 |
| guinn | 240 | 133 | W | W → D | 17 | 5-9 | 8-11 |
| guinn | 268 | 149 | W | W → D | 11 | 6-9 | 5-9 |
| guinn | 301 | 167 | B | D → W | 4 | 24-19 | 25-22 |
| guinn | 312 | 174 | W | D → W | 11 | 10-15 | 9-14 |


The retained KingsRow win is ACF 43 as White, game 78, ages 77/77; the earlier census wins on ACF 16 and 134 as White become draws after KingsRow's different choices. The earlier Cake ACF 143 Black win becomes a draw. GuiNN retains ten of the census's 22 wins, replaces twelve with draws and adds nine wins on formerly drawn keys.

For all opening/colour pairs, not only changed results, the first different move before the census-counterfactual boundary is:

| Opponent | First difference by Regicide | First difference by opponent | No differing move within the compared prefix |
|---|---|---|---|
| Cake | 3 | 190 | 119 |
| KingsRow | 5 | 204 | 103 |
| GuiNN | 3 | 250 | 59 |

"No differing move" includes identical sequences and one record ending earlier on its result/claim; it is not automatically the same termination or process state. Move identity uses from/to/captured-piece set after legal parsing, so alternate notation for the same capture is not counted as a variation. [mo2-comparisons.tsv](tables/mo2-comparisons.tsv) includes every key, both ages, census identifiers, prefix lengths, decision FEN and a separately labeled comparison with the full played-out census record. [mo2-changed-results.tsv](tables/mo2-changed-results.tsv) is the requested changed-result list with all FENs.

All 11 first differences by Regicide are searched moves, with positive node counts in both records; no book-move first difference is added by this comparison.

These comparisons do not isolate a cause: mo2 uses one long process history and claim endings, whereas the census used eight workers and continued play; elapsed search times and retained opponent state can also differ. The records show the changed choices and scores, not which experimental difference caused them.

## Evidence and reproduction

All detail is in this report and the linked small text files. Source revisions/final fetch and availability are in [source-heads.txt](evidence/source-heads.txt).

- [option_probe.py](fixtures/option_probe.py) and the three options tables exercise commands through our DLL in fresh processes, with and without the private adjacent database.
- Generate the reference adapter with REVIEW_OUT=reports/round8 and CHECKERBOARD_SRC pointing to the pinned public checkout. [time_adapter.py](fixtures/time_adapter.py), with [reference_adapter.py](fixtures/reference_adapter.py), extracts the actual pinned CheckerBoard search function and replaces only external UI/clock/getmove services; no GUI option or engine command implementation is substituted into our DLL.
- [api_terminal_clock.py](fixtures/api_terminal_clock.py), [api_cases.py](fixtures/api_cases.py), [terminal_clock.cpp](fixtures/terminal_clock.cpp) and [census_referee.cpp](fixtures/census_referee.cpp) produce the new short-mate case and the independent board result. The terminal 78-ply history is included.
- [draw_guard.cpp](fixtures/draw_guard.cpp), [reply_guard.cpp](fixtures/reply_guard.cpp), [runner_cases.py](fixtures/runner_cases.py), [host_arguments.py](fixtures/host_arguments.py), and the logging [mock_engine.cpp](fixtures/mock_engine.cpp) contain the bounded and scripted checks.
- [mo2_review.py](fixtures/mo2_review.py), [replay_records.cpp](fixtures/replay_records.cpp) and [ballots.tsv](fixtures/ballots.tsv) and the [round-7 census reference](fixtures/census-reference.tsv) reproduce the record replay/matching without an engine search.
- [claim-bench.log](evidence/claim-bench.log), [draw-guard.log](evidence/draw-guard.log), [before-api-test.log](evidence/before-api-test.log), [before-runner-movelimit.log](evidence/before-runner-movelimit.log) retain execution evidence.

Fixtures use the reports/round8 private review layout and the included prior histories/private test database path. No opponent engine or production launcher is needed to reproduce them. All committed files are text and below 1 MB.
