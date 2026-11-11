# Review A — Round 4

Date: 2026-11-11. Workspace: /home/admingod/regicide-review-a.

The missing old-order r11d/n8q PDNs arrived during this review in Book Lab commit 24cc6d8. The old-versus-match-order comparison is now complete for all three opponents. This report replaces the provisional Cake-only comparison with the actual old validation records; the E2 kept-process comparison is retained separately.

One finding concerns an optional combination of confirmation flags. The two earlier Book Lab fixes and the search statistics reset pass their original cases. The forcing-exchange calculation and generator checks below found no further defect.

## Versions and execution limits

I read docs/REVIEW_LEDGER.md from origin/master before reviewing, and again after fetching. Items already accepted, assigned, or fixed in that ledger are not new findings here.

| Ref | Head checked | Relevant source |
|---|---|---|
| origin/master | 12f0e09d034d09f4dbc9850f29f9f54cb235f940 | Engine code remains that of 1c6e997; later changes are ledger and runner restart-test additions |
| origin/t3/booklab | 223e6af72d2048d9841ddd1853bdf8d692f93aa2 | Guard/lock fix 33e8910, mining pairs 0fc0a0e, record additions bcf0323 and 24cc6d8 |
| origin/t3/search-lab | 846f3900eb7e714e1e69f8cf9ef2572bf787e9a9 | Search source is unchanged from e1a73bea130aaa6a049dec594effc3afe63b663d |
| origin/t3/truth-data | b6e49b61481df654e89003cff439da55ebaa62ea | datagen options, tests, E5 scripts and resume instructions |

All builds, tests, referee replays, mock runner invocations, and engine searches ran with nice -n 19. Builds used at most two jobs; tests and search fixtures used one engine/search thread. Commands finished in the foreground, each within ten minutes. No real match against another engine was run. I used only private source archives, builds, fixtures and output under this workspace. I did not run production launch/restart scripts, touch their locks, inspect or signal other agents' processes, or write in their directories. The exchange fixtures and generator tests did not need the shared endgame database.

“Ran” below means an executed private fixture or test. “Read” means source/committed record inspection only. Production DLLs, lock files and timing helpers were replaced by mocks where a control-flow fixture needed them.

## Finding 1 — confirmation accepts fresh processes between the two legs of a same-process pair

Files re-checked on Book Lab 223e6af (unchanged from 0fc0a0e):

- tools/booklab/wp12_confirm.py:130 combines --fresh-engines with --pairs-together.
- tools/booklab/wp12_confirm.py:244 propagates the same combination to stage 3.
- tools/booklab/wp12_confirm.py:287 accepts both flags without a compatibility check.

Concrete input to the command builder:

~~~python
match_cmd("DUMMY", private_deck, private_output, 1, True, True)
# conc=1, fresh=True, pairs=True
~~~

The deck contains one ordinary two-colour opening:

~~~text
B:W21-32:B1-12
~~~

I ran the returned command through the real match runner, replacing its two DLL/host specifications with private scripted protocol engines, and limiting each game to two plies. For both candidate and control, the White target game is round 1.2 but its PDN has EngineGamesA "0" and EngineGamesB "0". With fresh=False, the same White target is round 1.2 with ages "1" and "1". Thus --pairs --fresh-engines permits the confirmation run to count a White target whose preceding other-colour game was played by different processes.

For the requested kept-process match-order protocol, the first result does not meet the same-process condition. The option combination should either be refused for that protocol or explicitly treated as a different experiment. This is not an asymmetry between candidate and control: both arms get the same reset.

The complete executed matrix is [mining-pairs.tsv](tables/mining-pairs.tsv). It also establishes the intended Black-first ordering:

| Target colour | Fresh engines | Judged round | EngineGamesA/B | Earlier other-colour game in these processes |
|---|---|---|---|---|
| Black | no | 1.1 | 0 / 0 | no |
| White | no | 1.2 | 1 / 1 | yes |
| Black | yes | 1.1 | 0 / 0 | no |
| White | yes | 1.2 | 0 / 0 | no |

The original Round 4 request required a prelude for every candidate. A later lead clarification visible in the Book Lab thread at 09:25 UTC explicitly accepts Black candidates as the first game of their processes and says not to add the preceding ballot's pair. I therefore do not report Black-first ordering as a defect. The matrix makes the difference between that clarified rule and a universal-prelude rule explicit.

## 1. Book Lab fixes and mining order

### Completeness guard

Read: experiments/booklab/scripts/wp12_chain.sh. The guard now requires both a successful or legacy-missing exit_status and equal completed/scheduled counts:

~~~sh
{ [ ! -f $d/exit_status ] || [ "$(cat $d/exit_status)" = 0 ]; } &&
grep -Eq "^Games: ([0-9]+) of \1 scheduled" $d/summary.txt
~~~

Ran: experiments/booklab/scripts/t_guard2.sh with its temporary directory redirected inside my workspace.

| Fixture | Result |
|---|---|
| 156/156, exit 0 | accepted |
| 1/156, exit 0 — the original failing case | refused |
| 156/156, no exit_status | accepted |
| 1/156, no exit_status | refused |
| 156/156, exit 4 | refused |

The production guard fixes the reported case. Test coverage is limited: t_guard2.sh duplicates the predicate and prints its decisions; it has no assertions and does not invoke the production guard. Its exit status alone is not a regression check. The decision table above was checked against the expected output, rather than treating exit 0 as proof.

### Read-backs and timing lock

Read: wp12_confirm.py:176–202 and wp12_discover.py:279–311. Both now call flock(LOCK_EX) before a read-back loads either engine, and unlock on refusal.

Ran: the original confirmation ordering fixture, importing the new module and replacing every external path/action, including lock opening, flock, read-back, memory checks and child launch. Recorded events:

~~~text
lock
readback
readback
readback
launch
launch
unlock
~~~

The old order began with read-backs, before lock. The current order fixes that case. Discovery's equivalent order was read, not executed against its production paths.

Ran: tools/booklab/tests_wp12_gates.py, 22 tests passed.

### Pair construction and arm equality

Read and exercised with mock engines:

- Discovery --pairs writes an unrestricted ballot, uses concurrency 1 and --pairs-together, keeps both processes, plays our Black game first, and judges only the nominated colour.
- Confirmation stage 2 writes the same full-pair deck for candidate and control. Each block starts separate matches for the two arms. The candidate and control have the same number of workers, pair count, colour selection, clock, opponent settings and process-reset flags. The intended book/DLL difference is the treatment.
- With per_block=4, each arm has four workers and four pairs, giving four judged games per arm per block. The Black target is first in each process; the White target follows its other-colour game.
- Stage 3 writes full pairs for its contexts and forwards --pairs-together. It judges the context colour. There is no separate stage-3 baseline arm in this design; the direct candidate/control comparison is stage 2.
- pair_filter and stage-3 evaluation refuse to count a run if an unjudged companion game ended off the board. Discovery checks that its two games contain both colours, that Black came first, and that the companion ended on the board.

The scripts match the later Black-first clarification when fresh engines are disabled. Finding 1 describes the optional flag combination that breaks process continuity.

I only read experiments/booklab/scripts/t_pairs.py: it contains production-directory setup and a real pinwalk/database invocation, so I did not run it. The mock runner matrix above exercises the command construction without those actions.

## 2. Process state and the opponent's first varying move

### Records and method

The tagged corpus at 24cc6d8 contains 28 PDNs, 3,360 games and 342,887 plies: Cake 2,280 games, KingsRow 768, GuiNN 312. Every game was replayed by the independent referee. Every PDN result was independently matched to results.csv using the numeric Round pair and leg, not file order. There were no replay errors or off-board terminations in these records.

[record-sources.tsv](tables/record-sources.tsv) lists the 28 files. [source-integrity.tsv](tables/source-integrity.tsv) gives counts, age-tag form, exit status and SHA-256 of decompressed PDN text. The ten newly supplied old-validation files are checked separately in [old-order-source-integrity.tsv](tables/old-order-source-integrity.tsv).

For this process-state analysis I used only the tagged r11d/n8q E2 and match-order corpus. The older validation PDNs do not carry Worker/EngineGames tags and cannot reconstruct process history.

I grouped by opponent, opening, our colour and initial FEN. Within each group I built a tree of complete move prefixes, using referee-canonical moves, so different capture notation does not create a false difference. I did not merge transpositions. For each game, its “first varying opponent move” is the earliest opponent turn after the imposed three ballot plies at which at least two games reaching that exact prefix chose different moves. A later addition to the corpus can reveal an earlier varying move; in particular the enlarged KingsRow #71 corpus first varies at ply 21, rather than the ply-23 variation visible in the smaller initial subset.

Age is the opponent's EngineGamesB, or the older shared EngineGames tag. Values are completed real games before this game: 0, 1, or 2+. The launch warm-up search is not a real game in that counter. I reconstructed prior games within each run and Worker in numeric round order, requiring the previous age to be one lower. There were no history gaps. Separate A/B tags occur in 1,752 games and are equal throughout; 1,608 games have only the older shared counter, which cannot establish whether either engine restarted alone. No separate-tag record here exercises an asymmetric restart.

Previous-game categories are same opening/same colour, same opening/other colour, another opening, and no previous real game. The colour is Regicide's colour; opponent colour changes in the opposite way. The tables retain the exact previous game, opening and result, so the broad category is not the only available evidence.

### Overall association and limits

There are 2,538 first-variation game observations; these are games, not pairs of games.

| Opponent | First-variation observations | Different prefix positions | Positions with different choices and more than one age bucket | Observations at those positions | Positions still varying within one age/history category |
|---|---:|---:|---:|---:|---:|
| Cake | 1,782 | 243 | 239 | 1,759 | 219 |
| KingsRow | 756 | 306 | 3 | 150 | 306 |
| GuiNN | 0 | 0 | 0 | 0 | 0 |

For Cake, process age/history often changes together with move choice, but neither coarse age nor the previous-game category determines its move. For KingsRow, almost all comparable validation games have age 2+, so only three prefix positions provide variation across age buckets. GuiNN has only one tagged game per opening/colour in this committed corpus; no process-age comparison is possible. Zero observations there does not establish deterministic play.

These are descriptive comparisons. Phase, engine build, worker assignment, exact prior line, hash contents and timing are not independently randomized. I do not infer that age or the immediately previous game's colour causes a move or a result.

Complete evidence:

- [process-games.tsv](tables/process-games.tsv): every tagged game, age, worker, previous real game and category.
- [first-varying-move-games.tsv](tables/first-varying-move-games.tsv): every first-variation observation, choice, FEN and result.
- [first-varying-move-joint.tsv](tables/first-varying-move-joint.tsv): counts by exact prefix position, move, age bucket, previous-game category and experiment phase.

### Cake ACF 163, Regicide White

All 79 tagged games of this context reach the same first varying Cake turn at ply 15:

~~~text
B:W13,16,23,25,26,27,29,30,31,32:B1,2,3,4,6,7,10,11,14,20
~~~

| Age | Previous real game | Phase | 4-8 | 10-15 |
|---|---|---|---:|---:|
| 0 | none | fresh | 26 | 0 |
| 0 | none | kept old order | 8 | 0 |
| 1 | same opening, same colour | kept old order | 1 | 7 |
| 2+ | same opening, same colour | kept old order | 8 | 0 |
| 2+ | another opening | kept old order | 3 | 0 |
| 1 | same opening, other colour | natural pairs | 8 | 0 |
| 2+ | same opening, other colour | natural pairs | 16 | 0 |
| 2+ | same opening, other colour | match order | 2 | 0 |
| **Total** | | | **72** | **7** |

All seven 10-15 choices occur in the second real game of a kept process after the same opening and same colour. One of the eight such games instead chose 4-8. The seven 10-15 games have Regicide results 5 wins, 2 draws. The 72 games choosing 4-8 have 12 wins, 60 draws, including ten White wins in the natural-pair experiment. Thus “10-15 occurs only in that one process-history category” is true here; “all Cake losses need 10-15” is false.

The natural Cake run independently recounts to 10 wins, 38 draws, no losses across 48 games: Regicide Black 24 draws, White 10 wins and 14 draws.

### KingsRow ACF 71 and ACF 43

The first varying opponent turn for ACF 71, Regicide White, is ply 21:

~~~text
B:W22,23,25,27,28,31,32:B1,4,5,8,10,12,13
~~~

| Age/history | 8-11 | 5-9 |
|---|---:|---:|
| Age 0, no previous game | 32 | 0 |
| Age 1, same opening/same colour | 8 | 0 |
| Age 1, same opening/other colour | 8 | 0 |
| Age 2+, same opening/same colour | 8 | 0 |
| Age 2+, same opening/other colour | 13 | 5 |

This earlier choice alone does not separate the six wins: all six are in the age-1/other-colour row and all chose 8-11 there. At the known later ply 25, the six natural-pair wins all choose 10-14; that move appears only at age 1 after the other-colour game. The other two age-1/other-colour games choose 10-15 and draw. Fresh #71 and kept same-colour #71 each have 24 draws. Natural pairs have 6 wins and 42 draws across 48 games, with all wins as Regicide White.

[known-kingsrow-points.tsv](tables/known-kingsrow-points.tsv) gives the exact later FEN, move, age/history and result for every recorded game at the requested known plies. It is kept separate from the first-variation table.

For ACF 43, Regicide White, the first varying turn is ply 27:

~~~text
B:W21,22,23,26,28,30,31:B1,6,7,8,11,14,19
~~~

Ages 0 and 1 choose 8-12 in all 40 games. Age 2+ chooses 8-12 five times and 6-10 five times. All 50 games draw. At the earlier known ply 23, every one of these records chooses 8-11; the different move in an earlier book/net context is not reproduced here.

The third KingsRow age-informative position is ACF 71, Regicide Black, ply 20:

~~~text
W:W21,22,26,27,28,29,30,31:B1,2,5,9,10,11,12,14
~~~

Age 0 chooses 27-23 eight times. Age 2+ chooses 27-23 seven times, 27-24 five times, 28-24 five times and 26-23 once. This position also shows that age 2+ is too broad to determine a choice.

## 3. Old validation order versus match order, game by game

The ten old r11d/n8q validation PDNs supplied in 24cc6d8 contain 1,560 games and 178,111 plies. All replay legally and reconcile to their CSVs. I paired old and match-order games by opponent, repeat a/b, half-deck, ACF opening and Regicide colour. There is exactly one game per key in each arm.

Same book and net does not mean identical executable conditions: the old runs use the earlier runner/engine build; match-order runs use bl-1c6e997 and the newer runner. Source records and the Book Lab response explicitly identify that difference. These comparisons locate the first divergence and result changes; they do not isolate order as their cause.

| Opponent | Games compared | Old W/D/L | Match-order W/D/L | Identical whole sequence | First difference by opponent | First difference by Regicide | Result changed |
|---|---:|---|---|---:|---:|---:|---:|
| Cake | 624 | 0/624/0 | 0/624/0 | 213 | 401 | 10 | 0 |
| KingsRow | 624 | 0/624/0 | 1/623/0 | 12 | 608 | 4 | 1 |
| GuiNN | 312 | 20/292/0 | 27/285/0 | 35 | 276 | 1 | 29 |
| **Total** | **1,560** | **20/1,540/0** | **28/1,532/0** | **260** | **1,285** | **15** | **30** |

All 30 result-changing comparisons first diverge on the opponent's move. “First difference” compares canonical moves after an identical full prefix; the table gives the pre-move FEN. It does not claim that the first different move is the losing move.

[validation-old-vs-match-order.tsv](tables/validation-old-vs-match-order.tsv) contains all 1,560 comparisons, including unchanged results. [validation-result-changes.tsv](tables/validation-result-changes.tsv) contains the 30 changed games with IDs and FENs.

| Opponent/repeat | ACF | Our colour | Old → match result | First different ply | Old → match opponent move |
|---|---:|---|---|---:|---|
| KingsRow b | 16 | W | D → W | 41 | 32-28 → 9-14 |
| GuiNN a | 5 | W | D → W | 5 | 1-6 → 11-15 |
| GuiNN a | 6 | W | W → D | 7 | 1-5 → 6-9 |
| GuiNN a | 9 | W | W → D | 7 | 6-9 → 16-20 |
| GuiNN a | 10 | W | D → W | 17 | 1-6 → 4-8 |
| GuiNN a | 16 | W | D → W | 11 | 8-11 → 16-20 |
| GuiNN a | 23 | W | D → W | 5 | 11-15 → 10-14 |
| GuiNN a | 24 | W | W → D | 15 | 11-16 → 1-6 |
| GuiNN a | 31 | W | D → W | 11 | 8-11 → 15-19 |
| GuiNN a | 55 | W | D → W | 5 | 11-16 → 10-15 |
| GuiNN a | 60 | W | D → W | 19 | 4-8 → 11-15 |
| GuiNN a | 64 | W | D → W | 5 | 11-15 → 1-6 |
| GuiNN a | 65 | W | W → D | 11 | 15-19 → 3-7 |
| GuiNN a | 71 | W | D → W | 9 | 3-7 → 12-16 |
| GuiNN a | 74 | W | W → D | 11 | 6-10 → 11-16 |
| GuiNN a | 75 | W | D → W | 13 | 11-15 → 11-16 |
| GuiNN a | 87 | W | D → W | 21 | 1-6 → 7-10 |
| GuiNN a | 90 | W | W → D | 13 | 5-9 → 11-16 |
| GuiNN a | 92 | B | W → D | 4 | 17-14 → 25-22 |
| GuiNN a | 92 | W | W → D | 25 | 6-10 → 6-9 |
| GuiNN a | 96 | W | D → W | 15 | 9-13 → 1-5 |
| GuiNN a | 101 | W | W → D | 7 | 9-14 → 6-10 |
| GuiNN a | 112 | B | D → W | 10 | 28x19 → 27x20 |
| GuiNN a | 117 | W | D → W | 21 | 1-6 → 7-10 |
| GuiNN a | 121 | B | D → W | 4 | 18-14 → 27-23 |
| GuiNN a | 131 | W | W → D | 11 | 10-15 → 8-11 |
| GuiNN a | 133 | W | D → W | 21 | 8-12 → 16-19 |
| GuiNN a | 141 | W | W → D | 13 | 16-20 → 11-15 |
| GuiNN a | 153 | B | D → W | 18 | 29-25 → 17-14 |
| GuiNN a | 167 | B | D → W | 4 | 24-19 → 25-22 |

GuiNN has 18 draws becoming wins and 11 wins becoming draws. KingsRow repeat a is 312 draws in both arms; repeat b changes one draw to a win.

The preliminary Cake comparison used E2c kept-process repeats 1 and 2, not the old validation PDNs. That separate 624-game comparison remains correct: 218 identical sequences, 398 first different Cake moves, 8 first different Regicide moves, and zero result changes. It is retained in [cake-old-vs-match-order.tsv](tables/cake-old-vs-match-order.tsv), whose old_game identifiers explicitly start e2c-kept. The main table above uses the newly supplied v11d-n8q records.

There is no longer a missing-PDN gap for the requested ten old-validation versus ten match-order runs. A GuiNN age/history association remains unmeasurable because its old PDNs have no process tags and its tagged match-order run has no repeat at an opening/colour. The final fetch advanced Book Lab to 223e6af. It adds CSVs/summaries, but no matching PDNs, for e2c-fresh-3-h1/h2, e2d-kr134-fresh and e2d-kr43-natural. I independently recounted those CSVs: respectively 312 draws, 24 draws, and 1 win/47 draws. Their move choices and process tags cannot yet be checked. The new full-deck concurrency-1 experiments mentioned by the lead also have no committed PDNs in this snapshot and are not substituted for the requested comparisons.

## 4. Forcing-exchange pruning, mode 3, and statistics

### Meaning of the calculation

Read: src/search.cpp:24 and :580 on search-lab e1a73be, unchanged at 846f390.

After a candidate late quiet move, mode 3 generates all forced opponent captures. For each capture it generates all immediate capture replies, including full multi-jump paths and promotions, and computes:

~~~text
best number of opponent pieces we can capture immediately
minus number of our pieces the opponent just captured
~~~

It takes the minimum over the opponent's capture choices and exempts the quiet move from late-move/futility pruning only if that minimum is positive. Men and kings each count as one piece. There is no recursive search in this calculation. A promotion's future value is not material gained by this definition.

Mode 3 changes shallow late-move/futility pruning. It does not cancel late-move reductions; that separate exemption belongs to mode 2. The conversion-search exclusions and ordinary quiet-move prerequisites remain in force.

### Own exchange positions

Ran: private fixture using the real forcing_gain and Worker::search, with independently generated/refereed exchanges from tools/match's separate move generator. Black is the side making the listed quiet move.

| Case | Root FEN | Quiet move | Opponent captures / best immediate reply | Worst gain | Mode 3 |
|---|---|---|---|---:|---|
| One for one | B:W11,13,23:B1,5,17 | 5-9 | 13x6 takes 1; 1x10 takes 1 | 0 | remains prunable |
| Two for one, both choices good | B:W9,K15,16,20:B3,7,13 | 7-11 | 15x8, then 3x12x19; or 16x7, then 3x10x19; each 1 taken / 2 back | +1 | exempt |
| Several choices, one refutes the exchange | B:WK12,17,25,31:B1,8,K10,24,28 | 10-14 | 12x3 then 14x21x30 gives +1; 17x10 has no reply and gives -1 | -1 | remains prunable |
| Opponent capture promotes | B:W10,26,30:B1,K16,20,21,23 | 1-6 | 10x1 promotes, takes 1, no reply; 26x19x12 takes 2, 6x15 takes 1 | -1 | remains prunable |
| Our capture reply promotes | B:W10,26,30:B1,K16,20,21,23 | 21-25 | 30x21 then 23x30 promotes and gives 0; alternative 26x19x12 takes 2 with no reply | -2 | remains prunable |

All five calculated gains equal the independent oracle. In particular, the favourable capture option does not hide an unfavourable opponent choice, and promotion stops the capture sequence according to English rules.

To exercise the actual pruning gate, I searched each root through Worker::search at depth 1, non-PV, alpha/beta 20000/20001, HCE, database off, lmp_base=-100, no single-reply extension, with the target late in move order. In this deliberately narrow fixture:

| Case | Mode 1 LMP candidates / exemptions | Mode 3 LMP candidates / exemptions |
|---|---:|---:|
| One for one | 3 / 1 | 3 / 0 |
| Reply promotes | 7 / 5 | 7 / 0 |
| Opponent promotes | 7 / 5 | 7 / 0 |
| Two for one | 3 / 1 | 3 / 1 |
| Mixed choices | 8 / 7 | 8 / 0 |

I repeated these five gate cases with LMP disabled (lmp_depth=0, lmp_base=10000) and futility enabled (fut_depth=3, fut_margin=1). The futility candidate/exemption counters give the same five rows, including only the two-for-one exemption in mode 3.

The candidate counter counts a move when the ordinary pruning condition is met, before the exemption. The exemption counter counts moves retained by the selected mode. These are event counts over the search, not a count of distinct positions or exchange lines. Their placement and the executed cases agree with that meaning.

### Reset and mode-0 compatibility

Ran the original stale-statistics case: a search first populated the six counters (308,0,168,0,58,0); a subsequent table-only go returned zero nodes and all six counters zero. I also tested successive go calls without new_game in between: table-only, book-only, and no-legal-moves exits all returned zero counters. The reset now occurs at the start of Engine::go before the early returns.

Ran the single-thread bench with mode 0: all 17 per-position node counts, moves and scores exactly equal the saved master bench from Round 2, totaling **7,833,556 nodes**. Master search code has not changed since that comparison. The new counters do not change the searched tree when prune_forcing=0.

## 5. Datagen avoidance, parity and partial-file restart

Read: tools/datagen.cpp, tests/datagen_avoid_test.cmake and the committed E5 scripts on b6e49b6. Ran the committed test and the extra private cases below.

### What counts as a start, and whether parities can overlap

The avoidance key is the exact black bitboard, white bitboard, king bitboard and side to move of the first stored quiet position (the GAME_START record). It is not the raw random-opening board before any forced captures. A deterministic bit of the mixed key selects parity 0 or 1. The used set tests full-key equality; its hash-table hash does not replace equality.

Consequently, two generators configured with opposite parities cannot both store the same start key. A hash collision does not let the same key have two parities. This guarantee does not partition later positions or prevent transpositions. The random-opening draw itself is not partitioned: both generators can draw and play through the same unrecorded pre-capture start, with one later rejecting its first quiet position. Their stored GAME_START keys cannot overlap.

The --avoid files are read once at startup. They are not continuously tailed, so --avoid alone cannot stop concurrent same-parity generators from choosing the same new start after that snapshot. Within one process the used set is protected by a mutex; opposite parities provide the cross-process partition.

Ran: two finite 80-attempt runs, one worker each, nodes=300, seed=5, explicit --random-min 6 --random-max 16. They wrote 34 and 32 starts respectively, every start had the requested parity, and their exact-key intersection was empty. This is a finite check supporting the source-level partition argument.

### Committed avoidance test and direct repeat case

Ran the committed CMake test under nice -n 19, with DATAGEN pointing to my build and WORK under reports/round4. It reports:

~~~text
datagen --avoid: 27 new games, none with one of 11 avoided starts
(control repeated 11); --start-parity holds
~~~

The committed test exercises avoidance and parity membership, but not a truncated final game or both parity runs jointly.

For a direct causal avoidance check I used the quiet seed:

~~~text
B:W18,19,21,25,26,28,29,30,31:B1,2,3,4,5,6,7,9,12
~~~

With --games 1 --seed-plies 0, no avoid file produced one game and 68 records. Adding --avoid for the prior file containing that start produced zero games and zero positions. The output was empty. This verifies actual skipping rather than relying only on a changed random stream.

### Which statistics count a skipped start

A rejected start consumes a --games attempt. It increments used_starts_skipped or parity_skipped as appropriate. It does not increment the completed-game W/D/L counters or written-position count: the skip path continues before those updates.

It can still contribute to counters describing work already performed. Forced captures may be searched before the first quiet record where parity/avoidance is checked. Their incomplete-search or partial-loss work counters are not rolled back.

I verified both paths with a private diagnostic copy that only printed the existing internal counters:

- The direct quiet avoided seed consumes one attempted slot, increments used_skip by one, and leaves finished games, positions and end-reason counts zero.
- Starting from B:W16,21,22,23,25,26,27,28,29,30,31,32:B1-12 with nodes=1, then rejecting at the first quiet start, leaves completed games and positions zero while incomplete searches record the earlier capture searches. My two parity cases recorded 6 and 1 incomplete searches respectively.

The internal next_game atomic ends above the attempt budget because the loop increments it once more when testing for termination. I did not interpret that implementation detail as an extra played attempt.

### Restart with its own partly written file

I cut a generated file after the first complete 16-byte record of its last game, leaving an incomplete game block but no incomplete record. The retained file contains 11 GAME_STARTs. I restarted with that same path as --out and --avoid.

Ran result: existing bytes were preserved; 38 new starts were appended; none equals any of the 11 retained starts. The final incomplete game's marked start is therefore avoided even though its remaining records were missing. Avoidance needs the marked start, not an end-of-game marker.

I separately appended three stray bytes to make a partial record and tried the same restart. Datagen stopped with exit 1 and “partial record”, and the file was byte-for-byte unchanged. It does not append records at a misaligned offset.

### Random opening length 6–16

The changed production configuration is **RANDOM_PLIES=6-16**, forwarded by e5_gen.sh as --random-min 6 --random-max 16. RESUME.md states that restarts must retain it. Datagen's general default remains 4–12. Thus a bare datagen invocation does not silently adopt 6–16; the E5 configuration is the change under review.

Read: the inclusive random-length calculation spans both endpoints. The seeded half still adds 2–4 plies; RANDOM_PLIES changes the random-opening half only. A forced capture after that draw can move the first stored quiet position beyond ply 16, so stored-start depth is not a test of the random draw's upper bound.

Ran the start-pool dry check with --random-min 6 --random-max 16 --seed 21: 1,000 random-opening draws, 995 distinct boards. That is a draw-pool check, not a measurement after search-score filtering or parity selection.

The committed restart/launch scripts preserve the environment parameter when used as documented. I read them but did not execute them: they manage the protected training jobs and locks.

## Reproduction and retained evidence

Local detail, build logs, fixture sources, binary samples and raw replay caches remain under reports/round4 in my private directory. Only this report and the linked text evidence tables are published on t3/review-reports.

Ran: committed guard smoke script, 22 gate tests, mocked confirmation ordering fixture, eight mock pair-command combinations, five independent exchange cases plus real LMP and futility pruning-gate searches, early-return statistics tests, the 17-position mode-0 bench, committed datagen_avoid, opposite-parity/avoidance/partial-file fixtures, and independent legal replay/result reconciliation of 4,920 distinct source games (3,360 tagged plus 1,560 old validation).

Read only: production match launch/restart/timing scripts; discovery's timing-lock ordering; mining stage-3 launch propagation; the live-training configuration described in committed records. I did not run timed production matches or measure live opponent processes.

The tables use W/D/L from Regicide's perspective and colours B/W for Regicide unless a column explicitly says otherwise.
